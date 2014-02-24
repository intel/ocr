/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_MPI

#include "debug.h"

#include "ocr-sysboot.h"
#include "ocr-policy-domain.h"
#include "ocr-worker.h"

#include "utils/ocr-utils.h"

#include "mpi-comm-platform.h"
#include <mpi.h>

//TODO replace this with some INT_MAX from sal header
#include "limits.h"

#define DEBUG_TYPE COMM_PLATFORM


//
// MPI library Init/Finalize
//

/**
 * @brief Initialize the MPI library.
 */
void platformInitMPIComm(int argc, char ** argv) {
    int res = MPI_Init(&argc, &argv);
    ASSERT(res == MPI_SUCCESS);
}

/**
 * @brief Finalize the MPI library (no more remote calls after that).
 */
void platformFinalizeMPIComm() {
    int res = MPI_Finalize();
    ASSERT(res == MPI_SUCCESS);
}


//
// MPI communication implementation strategy
//

// Pre-post an irecv to listen to outstanding request and for every
// request that requires a response.
// Only supports fixed size receives.
#define STRATEGY_PRE_POST_RECV 0

// Use iprobe to scan for outstanding request (tag matches RECV_ANY_ID)
// and incoming responses for requests (using src/tag pairs)
#define STRATEGY_PROBE_RECV 1

// To tag outstanding send/recv
#define RECV_ANY_ID 0
#define SEND_ANY_ID 0

typedef struct {
    u64 msgId; // The MPI comm layer message id for this communication
    u32 properties;
    ocrPolicyMsg_t * msg;
    MPI_Request status;
#if STRATEGY_PROBE_RECV
    int src;
#endif
    u8 deleteSendMsg;
} mpiCommHandle_t;

static ocrLocation_t mpiRankToLocation(int mpiRank) {
    //DIST-TODO location: identity integer cast for now
    return (ocrLocation_t) mpiRank;
}

static int locationToMpiRank(ocrLocation_t location) {
    //DIST-TODO location: identity integer cast for now
    return (int) location;
}

/**
 * @brief Internal use - Returns a new message
 */
static ocrPolicyMsg_t * allocateNewMessage(ocrCommPlatform_t * self, u32 size) {
    ocrPolicyDomain_t * pd = self->pd;
    ocrPolicyMsg_t * message = pd->fcts.pdMalloc(pd, size);
    initializePolicyMessage(message, size);
    return message;
}

/**
 * @brief Internal use - Create a mpi handle to represent pending communications
 */
static mpiCommHandle_t * createMpiHandle(ocrCommPlatform_t * self, u64 id, u32 properties, ocrPolicyMsg_t * msg, u8 deleteSendMsg) {
    mpiCommHandle_t * handle = self->pd->fcts.pdMalloc(self->pd, sizeof(mpiCommHandle_t));
    handle->msgId = id;
    handle->properties = properties;
    handle->msg = msg;
    handle->deleteSendMsg = deleteSendMsg;
    return handle;
}

#if STRATEGY_PRE_POST_RECV
/**
 * @brief Internal use - Asks the comm-platform to listen for incoming communication.
 */
static void postRecvAny(ocrCommPlatform_t * self) {
    ocrCommPlatformMPI_t * mpiComm = (ocrCommPlatformMPI_t *) self;
    ocrPolicyMsg_t * msg = allocateNewMessage(self, mpiComm->maxMsgSize);
    mpiCommHandle_t * handle = createMpiHandle(self, RECV_ANY_ID, PERSIST_MSG_PROP, msg);
    void * buf = msg; // Reuse request message as receive buffer
    int count = mpiComm->maxMsgSize; // don't know what to expect, upper-bound on message size
    MPI_Datatype datatype = MPI_BYTE;
    int src = MPI_ANY_SOURCE;
#if STRATEGY_PROBE_RECV
    handle->src = MPI_ANY_SOURCE;
#endif
    int tag = RECV_ANY_ID;
    MPI_Comm comm = MPI_COMM_WORLD;
    DPRINTF(DEBUG_LVL_VERB,"[MPI %d] posting irecv ANY\n", mpiRankToLocation(self->pd->myLocation));
    int res = MPI_Irecv(buf, count, datatype, src, tag, comm, &(handle->status));
    ASSERT(res == MPI_SUCCESS);
    mpiComm->incoming->pushFront(mpiComm->incoming, handle);
}
#endif


//
// Communication API
//

u8 MPICommSendMessage(ocrCommPlatform_t * self,
                      ocrLocation_t target, ocrPolicyMsg_t * message,
                      u64 *id, u32 properties, u32 mask) {

    u64 bufferSize = message->bufferSize;
    ocrCommPlatformMPI_t * mpiComm = ((ocrCommPlatformMPI_t *) self);

    u64 baseSize = 0, marshalledSize = 0;
    ocrPolicyMsgGetMsgSize(message, &baseSize, &marshalledSize, MARSHALL_DBPTR | MARSHALL_NSADDR);
    u64 fullMsgSize = baseSize + marshalledSize;

    //DIST-TODO: multi-comm-worker: msgId incr only works if a single comm-worker per rank,
    //do we want OCR to provide PD, system level counters ?
    // Always generate an identifier for a new communication to give back to upper-layer
    u64 mpiId = mpiComm->msgId++;

    // If we're sending a request, set the message's msgId to this communication id
    if (message->type & PD_MSG_REQUEST) {
        message->msgId = mpiId;
    } else {
        // For response in ASYNC set the message ID as any.
        ASSERT(message->type & PD_MSG_RESPONSE);
        if (properties & ASYNC_MSG_PROP) {
            message->msgId = SEND_ANY_ID;
        }
        // else, for regular responses, just keep the original
        // message's msgId the calling PD is waiting on.
    }

    ocrPolicyMsg_t * messageBuffer = message;

    // Check if we need to allocate a new message buffer:
    //  - Does the serialized message fit in the current message ?
    //  - Is the message persistent (then need a copy anyway) ?
    bool deleteSendMsg = false;
    if ((fullMsgSize > bufferSize) || !(properties & PERSIST_MSG_PROP)) {
        // Allocate message and marshall a copy
        messageBuffer = allocateNewMessage(self, fullMsgSize);
        ocrPolicyMsgMarshallMsg(message, baseSize, (u8*)messageBuffer,
            MARSHALL_FULL_COPY | MARSHALL_DBPTR | MARSHALL_NSADDR);
        if (properties & PERSIST_MSG_PROP) {
            // Message was persistent, two cases:
            if ((properties & TWOWAY_MSG_PROP) && (!(properties & ASYNC_MSG_PROP))) {
                //  - The message is two-way and is not asynchronous: do not touch the
                //    message parameter, but record that we indeed made a new copy that
                //    we will have to deallocate when the communication is completed.
                deleteSendMsg = true;
            } else {
                //  - The message is one-way: By design, all one-way are heap-allocated copies.
                //    It is the comm-platform responsibility to free them, do it now since we've
                //    made our own copy.
                self->pd->fcts.pdFree(self->pd, message);
                message = NULL; // to catch misuses later in this function call
            }
        } else {
            // Message wasn't persistent, hence the caller is responsible for deallocation.
            // It doesn't matter whether the communication is one-way or two-way.
            properties |= PERSIST_MSG_PROP;
            ASSERT(false && "not used in current implementation (hence not tested)");
        }
    } else {
        // Marshall the message. We made sure we had enough space.
        ocrPolicyMsgMarshallMsg(messageBuffer, baseSize, (u8*)messageBuffer,
                                MARSHALL_APPEND | MARSHALL_DBPTR | MARSHALL_NSADDR);
    }

    // Warning: From now on, exclusively use 'messageBuffer' instead of 'message'
    ASSERT(fullMsgSize == messageBuffer->usefulSize);

    // Prepare MPI call arguments
    MPI_Datatype datatype = MPI_BYTE;
    int targetRank = locationToMpiRank(target);
    ASSERT(targetRank > -1);
    MPI_Comm comm = MPI_COMM_WORLD;

    // Setup request's MPI send
    mpiCommHandle_t * handle = createMpiHandle(self, mpiId, properties, messageBuffer, deleteSendMsg);

    // Setup request's response
    if ((messageBuffer->type & PD_MSG_REQ_RESPONSE) && !(properties & ASYNC_MSG_PROP)) {
    #if STRATEGY_PRE_POST_RECV
        // Reuse request message as receive buffer unless indicated otherwise
        ocrPolicyMsg_t * respMsg = messageBuffer;
        int respTag = mpiId;
        // Prepare a handle for the incoming response
        mpiCommHandle_t * respHandle = createMpiHandle(self, respTag, properties, respMsg);
        //PERF: (STRATEGY_PRE_POST_RECV) could do better if the response for this message's type is of fixed-length.
        int respCount = mpiComm->maxMsgSize;
        MPI_Request * status = &(respHandle->status);
        //Post a receive matching the request's msgId.
        //The other end will post a send using msgId as tag
        DPRINTF(DEBUG_LVL_VERB,"[MPI %d] posting irecv for msgId %lu\n", mpiRankToLocation(self->pd->myLocation), respTag);
        int res = MPI_Irecv(respMsg, respCount, datatype, targetRank, respTag, comm, status);
        if (res != MPI_SUCCESS) {
            //DIST-TODO define error for comm-api
            ASSERT(false);
            return res;
        }
        mpiComm->incoming->pushFront(mpiComm->incoming, respHandle);
    #endif
    #if STRATEGY_PROBE_RECV
        // In probe mode just record the recipient id to be checked later
        handle->src = targetRank;
    #endif
    }

    // If this send is for a response, use message's msgId as tag to
    // match the source recv operation that had been posted on the request send.
    // Note that msgId is set to SEND_ANY_ID a little earlier in the case of asynchronous
    // message like DB_ACQUIRE. It allows to handle the response as a one-way message that
    // is not tied to any particular request at destination
    int tag = (messageBuffer->type & PD_MSG_RESPONSE) ? messageBuffer->msgId : SEND_ANY_ID;

    MPI_Request * status = &(handle->status);

    DPRINTF(DEBUG_LVL_VVERB,"[MPI %d] posting isend for msgId=%lu msg=%p type=%x "
            "fullMsgSize=%lu marshalledSize=%lu to MPI rank %d\n",
            locationToMpiRank(self->pd->myLocation), messageBuffer->msgId,
            messageBuffer, messageBuffer->type, fullMsgSize, marshalledSize, targetRank);

    //If this assert bombs, we need to implement message chunking
    //or use a larger MPI datatype to send the message.
    ASSERT((fullMsgSize < INT_MAX) && "Outgoing message is too large");

    int res = MPI_Isend(messageBuffer, (int) fullMsgSize, datatype, targetRank, tag, comm, status);

    if (res == MPI_SUCCESS) {
        mpiComm->outgoing->pushFront(mpiComm->outgoing, handle);
        *id = mpiId;
    } else {
        //DIST-TODO define error for comm-api
        ASSERT(false);
    }

    return res;
}

u8 MPICommPollMessage_RL2(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                          u32 properties, u32 *mask) {
    // NOTE: If one-way were tracked by the comm-api we would need to have
    // this sort of loop there to notify message status.
    ocrCommPlatformMPI_t * mpiComm = ((ocrCommPlatformMPI_t *) self);
    ocrPolicyDomain_t * pd = self->pd;
    linkedlist_t * outbox = mpiComm->outgoing;
    // RL1 should only be outgoing shutdown message
    while(!outbox->isEmpty(outbox)) {
        iterator_t * outgoingIt = mpiComm->outgoingIt;
        outgoingIt->reset(outgoingIt);
        while (outgoingIt->hasNext(outgoingIt)) {
            mpiCommHandle_t * handle = (mpiCommHandle_t *) outgoingIt->next(outgoingIt);
            int completed = 0;
            int ret = MPI_Test(&(handle->status), &completed, MPI_STATUS_IGNORE);
            ASSERT(ret == MPI_SUCCESS);
            if(completed) {
                u32 msgProperties = handle->properties;
                ASSERT((handle->msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_MGT_SHUTDOWN);
                ASSERT(msgProperties & PERSIST_MSG_PROP);
                pd->fcts.pdFree(pd, handle->msg);
                pd->fcts.pdFree(pd, handle);
                outgoingIt->removeCurrent(outgoingIt);
            }
        }
    }
    return POLL_NO_MESSAGE;
}

#if STRATEGY_PROBE_RECV
u8 probeIncoming(ocrCommPlatform_t *self, int src, int tag, ocrPolicyMsg_t ** msg, int bufferSize) {
    //PERF: Would it be better to always probe and allocate messages for responses on the fly
    //rather than having all this book-keeping for receiving and reusing requests space ?
    //Sound we should get a pool of small messages (let say sizeof(ocrPolicyMsg_t) and allocate
    //variable size message on the fly).
    MPI_Status status;
    int available = 0;
    int success = MPI_Iprobe(src, tag, MPI_COMM_WORLD, &available, &status);
    ASSERT(success == MPI_SUCCESS);
    if (available) {
        ASSERT(msg != NULL);
        ASSERT((bufferSize == 0) ? ((tag == RECV_ANY_ID) && (*msg == NULL)) : 1);
        // Look at the size of incoming message
        MPI_Datatype datatype = MPI_BYTE;
        int count;
        success = MPI_Get_count(&status, datatype, &count);
        ASSERT(success == MPI_SUCCESS);
        ASSERT(count != 0);
        // Reuse request's or allocate a new message if incoming size is greater.
        if (count > bufferSize) {
            *msg = allocateNewMessage(self, count);
        }
        ASSERT(*msg != NULL);
        MPI_Comm comm = MPI_COMM_WORLD;
        success = MPI_Recv(*msg, count, datatype, src, tag, comm, MPI_STATUS_IGNORE);
        // After recv, the message size must be updated since it has just been overwritten.
        (*msg)->usefulSize = count;
        (*msg)->bufferSize = count;
        ASSERT(success == MPI_SUCCESS);

        // Unmarshall the message. We check to make sure the size is OK
        // This should be true since MPI seems to make sure to send the whole message
        u64 baseSize = 0, marshalledSize = 0;
        ocrPolicyMsgGetMsgSize(*msg, &baseSize, &marshalledSize, MARSHALL_DBPTR | MARSHALL_NSADDR);
        ASSERT((baseSize+marshalledSize) == count);
        // The unmarshalling is just fixing up fields to point to the correct
        // payload address trailing after the base message.
        // TODO: I'm thinking we can further customize un/marshalling for MPI. Because we use
        //       mpi tags, we actually don't need to send the header part of response message.
        //       We can directly recv the message at msg + header, update the msg header
        //       to be a response + flip src/dst.
        // TODO: See if we can improve unmarshalling by keeping around pointers for the various
        //       payload to be unmarshalled
        // TODO: We also need to deguidify all the fatGuids that are 'local' and decide
        //       where it is appropriate to do it.
        //       - REC: I think the right place would be in the user code (ie: not the comm layer)
        ocrPolicyMsgUnMarshallMsg((u8*)*msg, NULL, *msg,
                                  MARSHALL_APPEND | MARSHALL_NSADDR | MARSHALL_DBPTR);
        return POLL_MORE_MESSAGE;
    }
    return POLL_NO_MESSAGE;
}
#endif

u8 MPICommPollMessage_RL3(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                          u32 properties, u32 *mask) {
    ocrPolicyDomain_t * pd = self->pd;
    ocrCommPlatformMPI_t * mpiComm = ((ocrCommPlatformMPI_t *) self);

    ASSERT(msg != NULL);
    ASSERT((*msg == NULL) && "MPI comm-layer cannot poll for a specific message");

    // Iterate over outgoing communications (mpi sends)
    iterator_t * outgoingIt = mpiComm->outgoingIt;
    outgoingIt->reset(outgoingIt);
    while (outgoingIt->hasNext(outgoingIt)) {
        mpiCommHandle_t * mpiHandle = (mpiCommHandle_t *) outgoingIt->next(outgoingIt);
        int completed = 0;
        int ret = MPI_Test(&(mpiHandle->status), &completed, MPI_STATUS_IGNORE);
        ASSERT(ret == MPI_SUCCESS);
        if(completed) {
            DPRINTF(DEBUG_LVL_VVERB,"[MPI %d] sent msg=%p src=%d, dst=%d, msgId=%lu, type=0x%x, usefulSize=%lu\n",
                    locationToMpiRank(self->pd->myLocation), mpiHandle->msg,
                    locationToMpiRank(mpiHandle->msg->srcLocation), locationToMpiRank(mpiHandle->msg->destLocation),
                    mpiHandle->msg->msgId, mpiHandle->msg->type, mpiHandle->msg->usefulSize);
            u32 msgProperties = mpiHandle->properties;
            // By construction, either messages are persistent in API's upper levels
            // or they've been made persistent on the send through a copy.
            ASSERT(msgProperties & PERSIST_MSG_PROP);
            // Delete the message if one-way (request or response).
            // Otherwise message might be used to store the response later.
            if (!(msgProperties & TWOWAY_MSG_PROP) || (msgProperties & ASYNC_MSG_PROP)) {
                pd->fcts.pdFree(pd, mpiHandle->msg);
                pd->fcts.pdFree(pd, mpiHandle);
            } else {
                // The message requires a response, put it in the incoming list
                mpiComm->incoming->pushFront(mpiComm->incoming, mpiHandle);
            }
            outgoingIt->removeCurrent(outgoingIt);
        }
    }

    // Iterate over incoming communications (mpi recvs)
    iterator_t * incomingIt = mpiComm->incomingIt;
    incomingIt->reset(incomingIt);
#if STRATEGY_PRE_POST_RECV
    bool debugIts = false;
#endif
    while (incomingIt->hasNext(incomingIt)) {
        mpiCommHandle_t * mpiHandle = (mpiCommHandle_t *) incomingIt->next(incomingIt);
        //PERF: Would it be better to always probe and allocate messages for responses on the fly
        //rather than having all this book-keeping for receiving and reusing requests space ?
    #if STRATEGY_PROBE_RECV
        // Probe a specific incoming message. Response message overwrites the request one
        // if it fits. Otherwise, a new message is allocated. Upper-layers are responsible
        // for deallocating the request/response buffers.
        ocrPolicyMsg_t * reqMsg = mpiHandle->msg;
        u8 res = probeIncoming(self, mpiHandle->src, (int) mpiHandle->msgId, &mpiHandle->msg, mpiHandle->msg->bufferSize);
        // The message is properly unmarshalled at this point
        if (res == POLL_MORE_MESSAGE) {
            if ((reqMsg != mpiHandle->msg) && mpiHandle->deleteSendMsg) {
                // we did allocate a new message to store the response
                // and the request message was already an internal copy
                // made by the comm-platform, hence the pointer is only
                // known here and must be deallocated. The sendMessage
                // caller still has a pointer to the original message.
                pd->fcts.pdFree(pd, mpiHandle->msg);
            }
            ASSERT(reqMsg->msgId == mpiHandle->msgId);
            *msg = reqMsg;
            pd->fcts.pdFree(pd, mpiHandle);
            incomingIt->removeCurrent(incomingIt);
            return res;
        }
    #endif
    #if STRATEGY_PRE_POST_RECV
        debugIts = true;
        int completed = 0;
        int ret = MPI_Test(&(mpiHandle->status), &completed, MPI_STATUS_IGNORE);
        ASSERT(ret == MPI_SUCCESS);
        if (completed) {
            ocrPolicyMsg_t * receivedMsg = mpiHandle->msg;
            u32 needRecvAny = (receivedMsg->type & PD_MSG_REQUEST);
            DPRINTF(DEBUG_LVL_VERB,"[MPI %d] Received a message of type %x with msgId %d \n",
                    locationToMpiRank(self->pd->myLocation), receivedMsg->type, (int) receivedMsg->msgId);
            // if request : msg may be reused for the response
            // if response: upper-layer must process and deallocate
            //DIST-TODO there's no convenient way to let upper-layers know if msg can be reused
            *msg = receivedMsg;
            // We need to unmarshall the message here
            // Check the size for sanity (I think it should be OK but not sure in this case)
            u64 baseSize;
            ocrPolicyMsgGetMsgSize(*msg, &baseSize, &marshalledSize, MARSHALL_DBPTR | MARSHALL_NSADDR);
            ASSERT(baseSize + marshalledSize <= count);
            ocrPolicyMsgUnMarshallMsg((u8*)*msg, NULL, *msg,
                                      MARSHALL_APPEND | MARSHALL_DBPTR | MARSHALL_NSADDR);
            pd->fcts.pdFree(pd, mpiHandle);
            incomingIt->removeCurrent(incomingIt);
            if (needRecvAny) {
                // Receiving a request indicates a mpi recv any
                // has completed. Post a new one.
                postRecvAny(self);
            }
            return POLL_MORE_MESSAGE;
        }
    #endif
    }
#if STRATEGY_PRE_POST_RECV
    ASSERT(debugIts != false); // There should always be an irecv any posted
#endif
    u8 retValue = POLL_NO_MESSAGE;

#if STRATEGY_PROBE_RECV
    // Check for outstanding incoming. If any, a message is allocated
    // and returned through 'msg'.
    retValue = probeIncoming(self, MPI_ANY_SOURCE, RECV_ANY_ID, msg, 0);
    // Message is properly un-marshalled at this point
#endif

    return retValue;
}

u8 MPICommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                      u32 properties, u32 *mask) {
    ocrCommPlatformMPI_t * mpiComm = ((ocrCommPlatformMPI_t *) self);
    switch(mpiComm->rl) {
        case 3:
            return MPICommPollMessage_RL3(self, msg, properties, mask);
        case 2: {
            // The comm-worker is draining all remaining communication out
            // of the scheduler, then calls poll to complete those.
            //DIST-TODO stop: This RL's poll, block until all the work is completed so that
            //we guarantee the worker there's no outstanding comm
            u8 res = MPICommPollMessage_RL2(self, msg, properties, mask);
            ASSERT(res == POLL_NO_MESSAGE);
            mpiComm->rl_completed[2] = true;
            return res;
        }
        default:
            // nothing to do
            ASSERT(false && "Illegal RL reached in MPI-comm-platform pollMessage");
    }
    return POLL_NO_MESSAGE;
}

u8 MPICommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                      u32 properties, u32 *mask) {
    u8 ret = 0;
    do {
        ret = self->fcts.pollMessage(self, msg, properties, mask);
    } while(ret != POLL_MORE_MESSAGE);

    return ret;
}


//
// Comm-platform life-cycle
//

void MPICommBegin(ocrCommPlatform_t * self, ocrPolicyDomain_t * pd, ocrCommApi_t *commApi) {
    //Initialize base
    self->pd = pd;
    //DIST-TODO location: both commPlatform and worker have a location, are the supposed to be the same ?
    int nbRanks=0;
    int rank=0;
    MPI_Comm_size(MPI_COMM_WORLD, &nbRanks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    DPRINTF(DEBUG_LVL_VERB,"[MPI %d] comm-platform starts\n", rank);
    pd->myLocation = locationToMpiRank(rank);
    return;
}

void MPICommStart(ocrCommPlatform_t * self, ocrPolicyDomain_t * pd, ocrCommApi_t *commApi) {
    //DIST-TODO: multi-comm-worker: multi-initialization if multiple comm-worker
    //Initialize mpi comm internal queues
    ocrCommPlatformMPI_t * mpiComm = ((ocrCommPlatformMPI_t *) self);
    mpiComm->msgId = 1;
    mpiComm->incoming = newLinkedList(pd);
    mpiComm->outgoing = newLinkedList(pd);
    mpiComm->incomingIt = mpiComm->incoming->iterator(mpiComm->incoming);
    mpiComm->outgoingIt = mpiComm->outgoing->iterator(mpiComm->outgoing);

    // Default max size is customizable through setMaxExpectedMessageSize()
#if STRATEGY_PRE_POST_RECV
    //DIST-TODO STRATEGY_PRE_POST_RECV doesn't support arbitrary message size
    mpiComm->maxMsgSize = sizeof(ocrPolicyMsg_t)*2;
#endif
#if STRATEGY_PROBE_RECV
    // Do not need that with probe
    ASSERT(mpiComm->maxMsgSize == 0);
#endif
    // Generate the list of known neighbors (All-to-all)
    //DIST-TODO neighbors: neighbor information should come from discovery or topology description
    int nbRanks;
    MPI_Comm_size(MPI_COMM_WORLD, &nbRanks);
    pd->neighborCount = nbRanks - 1;
    pd->neighbors = pd->fcts.pdMalloc(pd, sizeof(ocrLocation_t) * pd->neighborCount);
    int myRank = (int) locationToMpiRank(pd->myLocation);
    int i = 0;
    while(i < (nbRanks-1)) {
        pd->neighbors[i] = mpiRankToLocation((myRank+i+1)%nbRanks);
        DPRINTF(DEBUG_LVL_VERB,"[MPI %d] Neighbors[%d] is %d\n", myRank, i, pd->neighbors[i]);
        i++;
    }

#if STRATEGY_PRE_POST_RECV
    // Post a recv any to start listening to incoming communications
    postRecvAny(self);
#endif
}

/**
 * @brief Stops the communication platform
 * Guarantees all outgoing sends and non-outstanding recv are done
 */
void MPICommStop(ocrCommPlatform_t * self) {
    ocrCommPlatformMPI_t * mpiComm = ((ocrCommPlatformMPI_t *) self);
    // Some other worker wants to stop the communication worker.
    //DIST-TODO stop: self stop for mpi-comm-platform i.e. called by comm-worker
    DPRINTF(DEBUG_LVL_VERB,"[MPI %d] Calling MPI STOP %d\n", locationToMpiRank(self->pd->myLocation), mpiComm->rl);
    switch(mpiComm->rl) {
        case 3:
            mpiComm->rl_completed[3] = true;
            mpiComm->rl = 2;
            // Transition to RL2:
            // At this point the comm-worker is in RL2 pulling and sending
            // all the remaining communication work from the scheduler.
            // There's no comm-platform code invocation in RL2.
        break;
        case 2:
            ASSERT(mpiComm->rl_completed[2]);
            mpiComm->rl = 1;
        break;
        case 1:
            //DIST-TODO stop: check if we remove a RL
            mpiComm->rl_completed[1] = true;
            mpiComm->rl = 0;
        break;
        case 0:
            // RL0: just cleanup data-structure pdMalloc-ed
            //NOTE: incoming may have recvs any posted
            mpiComm->incoming->destruct(mpiComm->incoming);
            ASSERT(mpiComm->outgoing->isEmpty(mpiComm->outgoing));
            mpiComm->outgoing->destruct(mpiComm->outgoing);
            mpiComm->incomingIt->destruct(mpiComm->incomingIt);
            mpiComm->outgoingIt->destruct(mpiComm->outgoingIt);

        break;
        default:
            ASSERT(false && "Illegal RL reached in MPI-comm-platform stop");
    }
    DPRINTF(DEBUG_LVL_VERB,"[MPI %d] Exiting Calling MPI STOP %d\n",  locationToMpiRank(self->pd->myLocation), mpiComm->rl);
}

void MPICommFinish(ocrCommPlatform_t *self) {
}


//
// Init and destruct
//

void MPICommDestruct (ocrCommPlatform_t * base) {
    //This should be called only once per rank and
    //by the same thread that did MPI_Init.
    platformFinalizeMPIComm();
    runtimeChunkFree((u64)base, NULL);
}

ocrCommPlatform_t* newCommPlatformMPI(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {
    ocrCommPlatformMPI_t * commPlatformMPI = (ocrCommPlatformMPI_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformMPI_t), NULL);
    //DIST-TODO location: what is a comm-platform location ? is it the same as the PD ?
    commPlatformMPI->base.location = ((paramListCommPlatformInst_t *)perInstance)->location;
    commPlatformMPI->base.fcts = factory->platformFcts;
    factory->initialize(factory, (ocrCommPlatform_t *) commPlatformMPI, perInstance);
    return (ocrCommPlatform_t*) commPlatformMPI;
}


/******************************************************/
/* MPI COMM-PLATFORM FACTORY                          */
/******************************************************/

void destructCommPlatformFactoryMPI(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

void initializeCommPlatformMPI(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * base, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, base, perInstance);
    ocrCommPlatformMPI_t * mpiComm = (ocrCommPlatformMPI_t*) base;
    mpiComm->msgId = 1; // all recv ANY use id '0'
    mpiComm->incoming = NULL;
    mpiComm->outgoing = NULL;
    mpiComm->incomingIt = NULL;
    mpiComm->outgoingIt = NULL;
    mpiComm->maxMsgSize = 0;
    int i = 0;
    while (i < (MPI_COMM_RL_MAX+1)) {
        mpiComm->rl_completed[i++] = false;
    }
    mpiComm->rl = MPI_COMM_RL_MAX;
}

ocrCommPlatformFactory_t *newCommPlatformFactoryMPI(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryMPI_t), (void *)1);
    base->instantiate = &newCommPlatformMPI;
    base->initialize = &initializeCommPlatformMPI;

    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryMPI);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), MPICommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrCommApi_t*), MPICommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*,ocrPolicyDomain_t*,
                                                  ocrCommApi_t*), MPICommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), MPICommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), MPICommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*,ocrLocation_t,
                                               ocrPolicyMsg_t*,u64*,u32,u32), MPICommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*,ocrPolicyMsg_t**,u32,u32*),
                                               MPICommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*,ocrPolicyMsg_t**,u32,u32*),
                                               MPICommWaitMessage);

    return base;
}

#endif /* ENABLE_COMM_PLATFORM_MPI */
