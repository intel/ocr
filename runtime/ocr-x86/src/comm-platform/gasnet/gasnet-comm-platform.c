/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMM_PLATFORM_GASNET

#include "debug.h"

#include "ocr-sysboot.h"
#include "ocr-policy-domain.h"
#include "ocr-worker.h"

#include "utils/ocr-utils.h"

#include "amhandler.h"
#include "gasnet-comm-platform.h"
#include <gasnet.h>

#define DEBUG_TYPE COMM_PLATFORM
#define MINHEAPOFFSET (128*4096)

// ---------------------------------
// OCR data structure
// ---------------------------------
typedef struct {
    u64 msgId; // The Gasnet comm layer message id for this communication
    u32 properties;
    ocrPolicyMsg_t * msg;
} gasnetCommHandle_t;

// ---------------------------------
// global variables
// ---------------------------------
// hack: store this communication platform pointer for incoming message
static ocrCommPlatform_t *platform;

// hack: storing the address of other nodes' segment
// instead of storing O(p), we should store O(log p) like CAF 2.0
static gasnet_seginfo_t *seginfo;

// ---------------------------------
// translate from OCR location to GASNet rank
// ---------------------------------
static inline int locationToGasnetRank(ocrLocation_t location) {
    return ((int) location);
}

static inline ocrLocation_t GasnetRankToLocation(int rank) {
  return (ocrLocation_t) rank;
}

// ---------------------------------
// debugging purposes
// ---------------------------------
static inline void debug(const char *msg) {
    DPRINTF(DEBUG_LVL_VVERB,"[%d]  %s \n", GasnetRankToLocation(gasnet_mynode()), msg);
    fflush(stdout);
}

// ---------------------------------
// Message allocation
// ---------------------------------
static ocrPolicyMsg_t * allocateNewMessage(ocrCommPlatform_t * self, u32 size) {
    ocrPolicyDomain_t * pd = self->pd;
    ocrPolicyMsg_t * message = pd->fcts.pdMalloc(pd, size);
    ASSERT(message != NULL);
    return message;
}

// ---------------------------------
// create GASNet communication handle
// ---------------------------------
static gasnetCommHandle_t * createGasnetHandle(ocrCommPlatform_t * self, u64 id, u32 properties, ocrPolicyMsg_t * msg) {
    ocrPolicyDomain_t * pd = self->pd;
    gasnetCommHandle_t * handle = pd->fcts.pdMalloc(pd, sizeof(gasnetCommHandle_t));
    ASSERT(handle != NULL);
    handle->msgId = id;
    handle->properties = properties;
    handle->msg = msg;
    return handle;
}

/***
 * to be called by an active message from remote process
 **/
static void gasnetMessageIncoming(ocrCommPlatform_t * self, ocrPolicyMsg_t *msg, uint64_t size) {
    ocrCommPlatformGasnet_t * gasnetComm = (ocrCommPlatformGasnet_t *) self;
    ocrPolicyMsg_t * buf = allocateNewMessage(self, size);

    gasnetCommHandle_t * handle = createGasnetHandle(self, 0, PERSIST_MSG_PROP, buf);

    debug(__func__);

    memcpy(buf, msg, size);
    DPRINTF(DEBUG_LVL_VERB,"[%d][GASNET] Received a message of type %x with msgId %d \n",
            (int) self->pd->myLocation, msg->type, (int) msg->msgId);
    //VC8 put a pthread lock here: I'm not sure this can be invoked concurrently or not
#ifdef CONCURRENT_GASNET
    hal_lock32(&gasnetComm->queueLock);
#endif
    gasnetComm->incoming->pushFront(gasnetComm->incoming, handle);
#ifdef CONCURRENT_GASNET
    hal_unlock32(&gasnetComm->queueLock);
#endif
}


// ---------------------------------
// Active messages definition
// ---------------------------------
/**
 * function to be invoked for medium-size message
 **/
static void gasnetAMMessageMedium(gasnet_token_t token, void *buf, size_t nbytes, gasnet_handlerarg_t arg)
{
  gasnet_node_t src;
  GASNET_Safe( gasnet_AMGetMsgSource(token, &src) );
  DPRINTF(DEBUG_LVL_VVERB, "[%d] %s: platform=%p from %d, size: %d\n",
          GasnetRankToLocation(gasnet_mynode()), __func__, platform, src, nbytes);

  gasnetMessageIncoming(platform, (ocrPolicyMsg_t *)buf, nbytes);
}

/**
 * Function to be invoked for long-size message
 * A long message communication is not as efficient as the medium one.
 * we need to avoid as much as possible using a long message.
 * The future version of Gasnet should support asynchrony AM Long.
 **/
static void gasnetAMMessageLong(gasnet_token_t token, void *buf, size_t nbytes, gasnet_handlerarg_t arg)
{
  gasnet_node_t src;
  GASNET_Safe( gasnet_AMGetMsgSource(token, &src) );
  DPRINTF(DEBUG_LVL_VVERB, "[%d] %s: platform=%p from %d, size: %d\n",
          GasnetRankToLocation(gasnet_mynode()), __func__, platform, src, nbytes);

  gasnetMessageIncoming(platform, (ocrPolicyMsg_t *)buf, nbytes);
}

// ---------------------------------
// declaring the function shippings
// ---------------------------------
AMHANDLER_REGISTER(gasnetAMMessageMedium);
AMHANDLER_REGISTER(gasnetAMMessageLong);

// ---------------------------------
// amhandler_register_invoke: function to invoke function shippings
// ---------------------------------
#define AMHANDLER_ENTRY(handler)  AMH_REGISTER_FCN(handler)();
static void
amhandler_register_invoke(void)
{
AMHANDLER_ENTRY(gasnetAMMessageMedium);
AMHANDLER_ENTRY(gasnetAMMessageLong);
}
#undef AMHANDLER_ENTRY


// ---------------------------------
// platform initializaton
// ---------------------------------
void platformInitGasnetComm(int argc, char ** argv) {

  gasnet_handlerentry_t * table;
  int count = 0;
  uintptr_t maxLocalSegSize;

  // initialize GASNet
  GASNET_Safe( gasnet_init(&argc, &argv) ) ;

  // initialize AM functions
  amhandler_register_invoke();
  table = amhandler_table();
  count = amhandler_count();

  maxLocalSegSize = gasnet_getMaxLocalSegmentSize();
  int segSize = GASNET_PAGESIZE * 64;
  segSize = (maxLocalSegSize<segSize ? maxLocalSegSize : segSize);
  int minheap = MINHEAPOFFSET;

  GASNET_Safe( gasnet_attach(table, count, segSize, minheap));

  gasnet_node_t nodes = gasnet_nodes();

  // get the address of other nodes
  seginfo = (gasnet_seginfo_t*) malloc(nodes * sizeof(gasnet_seginfo_t));
  GASNET_Safe( gasnet_getSegmentInfo(seginfo, nodes) );

  debug(__func__);
}

// ---------------------------------
// platform finalization
// ---------------------------------
void platformFinalizeGasnetComm() {
  debug(__func__);

  /* Spec says client should include a barrier before gasnet_exit() */
  gasnet_barrier_notify(0,GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait(0,GASNET_BARRIERFLAG_ANONYMOUS);

  free(seginfo);

  gasnet_exit(0);
}

//-------------------------------------------------------------------------------------------
// Active message for receiving messages
//-------------------------------------------------------------------------------------------
// short messages

u8 GasnetCommSendMessage(ocrCommPlatform_t * self,
                      ocrLocation_t target, ocrPolicyMsg_t * message,
                      u64 bufferSize, u64 *id, u32 properties, u32 mask) {

    ocrCommPlatformGasnet_t * gasnetComm = ((ocrCommPlatformGasnet_t *) self);
    u64 gasnetId = gasnetComm->msgId++;

    if (message->type & PD_MSG_REQUEST) {
      message->msgId = gasnetId;
    }
    int targetRank = locationToGasnetRank(target);
    ASSERT(targetRank > -1);
    DPRINTF(DEBUG_LVL_VVERB, "[%d] GasnetCommSendMessage self=%p, to %d (%d) %d bytes\n",
            (int) self->pd->myLocation, self, target, targetRank, bufferSize);

    // ------------------------------------------------------------
    // if the length of the message is less than the maximum of medium gasnet,
    // we'll use gasnet_AMRequestMedium1.
    // if the length is bigger then medium size and less than long size,
    // we'll use gasnet_AMRequestLong1 (which use address in segment)
    // Otherwise, we send an error message.
    // ------------------------------------------------------------

    if (gasnet_AMMaxMedium() > bufferSize) {
      // short and medium message
      //VC8 when this call terminates, does message is still potentially used by the gasnet runtime or not ?
      GASNET_Safe( gasnet_AMRequestMedium1(targetRank, AMHANDLER(gasnetAMMessageMedium), message, bufferSize,
                                           gasnetId) );

    } else if (gasnet_AMMaxLongRequest() > bufferSize) {
      // in case of long message, use AM's long request
      void *address = seginfo[targetRank].addr;
      GASNET_Safe( gasnet_AMRequestLong1(targetRank, AMHANDLER(gasnetAMMessageLong), message, bufferSize,
                                         address, gasnetId) );

    } else {
      // at the moment we couldn't afford to have a huge message due to Gasnet limitation.
      // we'll deal with this by chunking the message.
      DPRINTF(DEBUG_LVL_VVERB, "[%d] %s ERROR too long message (%d bytes)\n", (int) self->pd->myLocation, __func__, bufferSize);
      ASSERT(false);
      // TODO: the buffer size if too big to send. We need to split it.
      return 1;
    }

    DPRINTF(DEBUG_LVL_VERB,"[%d][GASNET] AM for msgId %ld type %x to rank %d\n", (int) self->pd->myLocation, message->msgId, message->type, targetRank);
    *id = gasnetId;

    return 0;
}


//-------------------------------------------------------------------------------------------
// Comm life-cycle functions
//-------------------------------------------------------------------------------------------
//


//-------------------------------------------------------------------------------------------
// polling message for the third attempt
//-------------------------------------------------------------------------------------------
u8 GasnetCommPollMessage_RL3(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                      u64* bufferSize, u32 properties, u32 *mask) {

  ASSERT(msg != NULL);
  ASSERT(*msg == NULL && "GASNet cannot poll for a specific message");

  ocrCommPlatformGasnet_t *gasnetComm = (ocrCommPlatformGasnet_t*) self;
  ocrPolicyDomain_t *pd  = self->pd;

  // ---------------------------------------
  // remove all outgoing tasks in the queue
  // ---------------------------------------
  // VC8 I don't understand why we need that in gasnet
  // iterator_t *iterator = gasnetComm->outgoing->iterator(gasnetComm->outgoing);
  // while(iterator->hasNext(iterator)) {
  //   // check the queue
  //   //VC8 From what I read on the net, this thing allows to call the handler of an AM. Is that right ?
  //   GASNET_Safe(gasnet_AMPoll());

  //   //VC8 I don't understand how this AMPoll is related to this particular instance of gasnetCommHandle_t in the outgoing list.
  //   gasnetCommHandle_t *handle = (gasnetCommHandle_t*) iterator->next(iterator);
  //   u32 msgProperties = handle->properties;
  //   ASSERT(msgProperties & PERSIST_MSG_PROP);
  //   if (!(msgProperties & TWOWAY_MSG_PROP)) {
  //     pd->fcts.pdFree(pd, handle->msg);
  //   }
  //   pd->fcts.pdFree(pd, handle);
  //   iterator->removeCurrent(iterator);
  // }
  // iterator->destruct(iterator);

  // ---------------------------------------
  // remove all incoming tasks in the queue
  // ---------------------------------------
  // check the queue
  GASNET_Safe(gasnet_AMPoll());

#ifdef CONCURRENT_GASNET
  hal_lock32(&gasnetComm->queueLock);
#endif
  iterator_t * iterator = gasnetComm->incoming->iterator(gasnetComm->incoming);
  while(iterator->hasNext(iterator)) {
    // The handle the AM handle has created
    //VC8: it sounds that in gasnet we don't need to create a handle on incoming, we could just
    //store the message in the queue and give it out to the caller
    gasnetCommHandle_t *handle = (gasnetCommHandle_t*) iterator->next(iterator);
    ocrPolicyMsg_t * receivedMsg = handle->msg;

    //VC8: don't understand why you would need that in gasnet, the AM handler is always listening right ?
    // if (receivedMsg->type & PD_MSG_REQUEST) {
    //   // Receiving a request indicates a mpi recv any
    //   // has completed. Post a new one.
    //   DPRINTF(DEBUG_LVL_VVERB, "[%d] WARNING: a request message still in wait self=%p\n", gasnet_mynode(), self);
    //   ocrPolicyMsg_t *msg = allocateNewMessage(self, gasnetComm->maxMsgSize);
    //   gasnetCommHandle_t *handle = createGasnetHandle(self, 0, PERSIST_MSG_PROP, msg);
    //   gasnetComm->incoming->pushFront(gasnetComm->incoming, handle);
    // }
    *msg = receivedMsg;
    pd->fcts.pdFree(pd, handle);
    iterator->removeCurrent(iterator);
    iterator->destruct(iterator);
#ifdef CONCURRENT_GASNET
    hal_unlock32(&gasnetComm->queueLock);
#endif
    return POLL_MORE_MESSAGE;
  }
  iterator->destruct(iterator);
#ifdef CONCURRENT_GASNET
  hal_unlock32(&gasnetComm->queueLock);
#endif
  return POLL_NO_MESSAGE;
}



//-------------------------------------------------------------------------------------------
// polling message for the second attempt
//-------------------------------------------------------------------------------------------
u8 GasnetCommPollMessage_RL2(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                      u64* bufferSize, u32 properties, u32 *mask) {

  ocrCommPlatformGasnet_t *gasnetComm = (ocrCommPlatformGasnet_t*) self;
  ocrPolicyDomain_t *pd  = self->pd;
  linkedlist_t * outbox = gasnetComm->outgoing;

  while(!outbox->isEmpty(outbox)) {
    iterator_t *iterator = outbox->iterator(outbox);

    while(iterator->hasNext(iterator)) {
      // check the queue
      GASNET_Safe(gasnet_AMPoll());

      gasnetCommHandle_t *handle = (gasnetCommHandle_t*) iterator->next(iterator);
      pd->fcts.pdFree(pd, handle->msg);
      pd->fcts.pdFree(pd, handle);
      iterator->removeCurrent(iterator);
    }
    iterator->destruct(iterator);
  }
  return POLL_NO_MESSAGE;
}


u8 GasnetCommPollMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                      u64* bufferSize, u32 properties, u32 *mask) {
    ocrCommPlatformGasnet_t * gasnetComm = ((ocrCommPlatformGasnet_t *) self);
    // debug(__func__);
    switch(gasnetComm->rl) {
    case 3:
        return GasnetCommPollMessage_RL3(self, msg, bufferSize, properties, mask);
    case 2:
    default:
        GasnetCommPollMessage_RL2(self, msg, bufferSize, properties, mask);
        gasnetComm->rl_completed[2] = true;
    }
    return POLL_NO_MESSAGE;

}

u8 GasnetCommWaitMessage(ocrCommPlatform_t *self, ocrPolicyMsg_t **msg,
                      u64* bufferSize, u32 properties, u32 *mask) {
    u8 ret = 0;
    debug(__func__);
    do {
        ret = self->fcts.pollMessage(self, msg, bufferSize, properties, mask);
    } while(ret != POLL_MORE_MESSAGE);

    return ret;
}

void GasnetCommBegin(ocrCommPlatform_t * self, ocrPolicyDomain_t * pd, ocrCommApi_t *commApi) {
    debug(__func__);

    self->pd = pd;
    int rank=gasnet_mynode();
    pd->myLocation = GasnetRankToLocation(rank);
    platform = self;
}

void GasnetCommStart(ocrCommPlatform_t * self, ocrPolicyDomain_t * pd, ocrCommApi_t *commApi) {
    ocrCommPlatformGasnet_t * gasnetComm = ((ocrCommPlatformGasnet_t *) self);
    debug(__func__);
    gasnetComm->msgId = 1;
    gasnetComm->incoming = newLinkedList(pd);
    gasnetComm->outgoing = newLinkedList(pd);

    gasnetComm->maxMsgSize = sizeof(ocrPolicyMsg_t);

    // All-to-all neighbor knowledge
    int nbRanks = gasnet_nodes();
    pd->neighborCount = nbRanks - 1;
    pd->neighbors = pd->fcts.pdMalloc(pd, sizeof(ocrLocation_t) * pd->neighborCount);
    int myRank = (int) locationToGasnetRank(pd->myLocation);
    int i = 0;
    while(i < (nbRanks-1)) {
        pd->neighbors[i] = GasnetRankToLocation((myRank+i+1)%nbRanks);
        DPRINTF(DEBUG_LVL_VERB,"[%d] neighbors[%d] is %d\n", pd->myLocation , i, pd->neighbors[i]);
        i++;
    }
#ifdef CONCURRENT_GASNET
    gasnetComm->queueLock = 0;
#endif
    GASNET_Safe(gasnet_AMPoll());
}

void GasnetCommStop(ocrCommPlatform_t * self) {
    ocrCommPlatformGasnet_t * gasnetComm = ((ocrCommPlatformGasnet_t *) self);

    debug(__func__);
    switch(gasnetComm->rl) {
    case 3:
        gasnetComm->rl_completed[3] = true;
        gasnetComm->rl = 2;
        break;
    case 2:
        ASSERT(gasnetComm->rl_completed[2]);
        gasnetComm->rl = 1;
        break;
    case 1:
        gasnetComm->rl_completed[1] = true;
        gasnetComm->rl = 0;
        break;
    case 0:
        gasnetComm->incoming->destruct(gasnetComm->incoming);
        ASSERT(gasnetComm->outgoing->isEmpty(gasnetComm->outgoing));
        gasnetComm->outgoing->destruct(gasnetComm->outgoing);
        break;
    default:
        ASSERT(false && "Illegal RL reached in Gasnet-comm-platform stop");
    }
    DPRINTF(DEBUG_LVL_VVERB,"[%d] Exiting Calling Gasnet STOP %d\n",  (int) self->pd->myLocation, gasnetComm->rl);
}


void GasnetCommFinish(ocrCommPlatform_t *self) {
    debug(__func__);
}


void GasnetCommDestruct(ocrCommPlatform_t * base) {
    platformFinalizeGasnetComm();
    runtimeChunkFree((u64)base, NULL);
}


ocrCommPlatform_t* newCommPlatformGasnet(ocrCommPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCommPlatformGasnet_t * commPlatformGasnet = (ocrCommPlatformGasnet_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformGasnet_t), NULL);

    commPlatformGasnet->base.location = ((paramListCommPlatformInst_t *)perInstance)->location;
    commPlatformGasnet->base.fcts = factory->platformFcts;
    factory->initialize(factory, (ocrCommPlatform_t *) commPlatformGasnet, perInstance);
    return (ocrCommPlatform_t*) commPlatformGasnet;
}



/******************************************************/
/*  GASNET COMM-PLATFORM FACTORY                      */
/******************************************************/

 void destructCommPlatformFactoryGasnet(ocrCommPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

void initializeCommPlatformGasnet(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * base, ocrParamList_t * perInstance) {
    initializeCommPlatformOcr(factory, base, perInstance);
    ocrCommPlatformGasnet_t * gasnetComm = (ocrCommPlatformGasnet_t*) base;
    gasnetComm->msgId = 1;
    gasnetComm->incoming = NULL;
    gasnetComm->outgoing = NULL;
    gasnetComm->maxMsgSize = 0;
    int i = 0;
    while (i < (GASNET_COMM_RL_MAX+1)) {
        gasnetComm->rl_completed[i++] = false;
    }
    gasnetComm->rl = GASNET_COMM_RL_MAX;
}

ocrCommPlatformFactory_t *newCommPlatformFactoryGasnet(ocrParamList_t *perType) {
    ocrCommPlatformFactory_t *base = (ocrCommPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCommPlatformFactoryGasnet_t), (void *)1);
    base->instantiate = &newCommPlatformGasnet;
    base->initialize = &initializeCommPlatformGasnet;

    base->destruct = FUNC_ADDR(void (*)(ocrCommPlatformFactory_t*), destructCommPlatformFactoryGasnet);
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCommPlatform_t*), GasnetCommDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCommPlatform_t*, ocrPolicyDomain_t*,
                                                  ocrCommApi_t*), GasnetCommBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCommPlatform_t*,ocrPolicyDomain_t*,
                                                  ocrCommApi_t*), GasnetCommStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCommPlatform_t*), GasnetCommStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCommPlatform_t*), GasnetCommFinish);
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*,ocrLocation_t,
                                               ocrPolicyMsg_t*,u64,u64*,u32,u32), GasnetCommSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*,ocrPolicyMsg_t**,u64*,u32,u32*),
                                               GasnetCommPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCommPlatform_t*,ocrPolicyMsg_t**,u64*,u32,u32*),
                                               GasnetCommWaitMessage);

    return base;
}


#endif //ENABLE_COMM_PLATFORM_GASNET
