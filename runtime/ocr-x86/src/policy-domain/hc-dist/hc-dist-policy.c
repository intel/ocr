
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_HC_DIST

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "experimental/ocr-placer.h"
#include "utils/hashtable.h"
#include "utils/queue.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "policy-domain/hc-dist/hc-dist-policy.h"

//DIST-TODO sep-concern: bad ! This is to use a worker's wid to map to a comm-api
#include "worker/hc/hc-worker.h"
//DIST-TODO cloning: sep-concern: need to know end type to support edt templates cloning
#include "task/hc/hc-task.h"
#include "event/hc/hc-event.h"

#define DEBUG_TYPE POLICY

#define RETRIEVE_LOCATION_FROM_MSG(pd, fname, dstLoc) \
    ocrFatGuid_t fatGuid__ = PD_MSG_FIELD(fname); \
    u8 res = guidLocation(pd, fatGuid__, &dstLoc); \
    ASSERT(!res);

#define RETRIEVE_LOCATION_FROM_GUID_MSG(pd, dstLoc) \
    ocrFatGuid_t fatGuid__ = PD_MSG_FIELD(guid); \
    u8 res = guidLocation(pd, fatGuid__, &dstLoc); \
    ASSERT(!res);

#define RETRIEVE_LOCATION_FROM_GUID(pd, dstLoc, guid__) \
    ocrFatGuid_t fatGuid__; \
    fatGuid__.guid = guid__; \
    fatGuid__.metaDataPtr = NULL; \
    u8 res = guidLocation(pd, fatGuid__, &dstLoc); \
    ASSERT(!res);

#define PROCESS_MESSAGE_RETURN_NOW(pd, retCode) \
    hcDistReleasePd((ocrPolicyDomainHc_t *) pd); \
    return retCode;


/****************************************************/
/* PROXY DATABLOCK MANAGEMENT                       */
/****************************************************/

//DIST-TODO: This is a poor's man meta-data cloning for datablocks.
// May be part of the datablock implementation at some point.

/**
 * @brief State of a Proxy for a DataBlock
 */
typedef enum {
    PROXY_DB_CREATED,   /**< The proxy DB has been created and is registered in GUID provider */
    PROXY_DB_FETCH,     /**< The DB ptr is being fetch */
    PROXY_DB_RUN,       /**< The DB ptr is being used */
    PROXY_DB_RELINQUISH /**< The DB ptr is being released (possibly incuring Write-Back) */
} ProxyDbState_t;

//Default proxy DB internal queue size
#define PROXY_DB_QUEUE_SIZE_DEFAULT 4

/**
 * @brief Data-structure to store foreign DB information
 */
typedef struct {
    ProxyDbState_t state;
    u32 nbUsers;
    u32 refCount;
    u32 lock;
    Queue_t * acquireQueue;
    u16 mode;
    u64 size;
    void * volatile ptr;
    u32 flags;
} ProxyDb_t;

/**
 * @brief Allocate a proxy DB
 */
static ProxyDb_t * createProxyDb(ocrPolicyDomain_t * pd) {
    ProxyDb_t * proxyDb = pd->fcts.pdMalloc(pd, sizeof(ProxyDb_t));
    proxyDb->state = PROXY_DB_CREATED;
    proxyDb->nbUsers = 0;
    proxyDb->refCount = 0;
    proxyDb->lock = 0;
    proxyDb->acquireQueue = NULL;
    // Cached DB information
    proxyDb->mode = 0;
    proxyDb->size = 0;
    proxyDb->ptr = NULL;
    proxyDb->flags = 0;
    return proxyDb;
}

/**
 * @brief Reset a proxy DB.
 * Warning: This call does NOT reinitialize all of the proxy members !
 */
static void resetProxyDb(ProxyDb_t * proxyDb) {
    proxyDb->state = PROXY_DB_CREATED;
    proxyDb->mode = 0;
    proxyDb->flags = 0;
    proxyDb->nbUsers = 0;
    // DBs are not supposed to be resizable hence, do NOT reset
    // size and ptr so they can be reused in the subsequent fetch.
}

/**
 * @brief Lookup a proxy DB in the GUID provider.
 *        Increments the proxy's refCount by one.
 * @param dbGuid            The GUID of the datablock to look for
 * @param createIfAbsent    Create the proxy DB if not found.
 */
static ProxyDb_t * getProxyDb(ocrPolicyDomain_t * pd, ocrGuid_t dbGuid, bool createIfAbsent) {
    hal_lock32(&((ocrPolicyDomainHcDist_t *) pd)->lockDbLookup);
    ProxyDb_t * proxyDb = NULL;
    u64 val;
    pd->guidProviders[0]->fcts.getVal(pd->guidProviders[0], dbGuid, &val, NULL);
    if (val == 0) {
        if (createIfAbsent) {
            proxyDb = createProxyDb(pd);
            pd->guidProviders[0]->fcts.registerGuid(pd->guidProviders[0], dbGuid, (u64) proxyDb);
        } else {
            hal_unlock32(&((ocrPolicyDomainHcDist_t *) pd)->lockDbLookup);
            return NULL;
        }
    } else {
        proxyDb = (ProxyDb_t *) val;
    }
    proxyDb->refCount++;
    hal_unlock32(&((ocrPolicyDomainHcDist_t *) pd)->lockDbLookup);
    return proxyDb;
}

/**
 * @brief Release a proxy DB, decrementing its refCount counter by one.
 * Warning: This is different from releasing a datablock.
 */
static void relProxyDb(ocrPolicyDomain_t * pd, ProxyDb_t * proxyDb) {
    hal_lock32(&((ocrPolicyDomainHcDist_t *) pd)->lockDbLookup);
    proxyDb->refCount--;
    hal_unlock32(&((ocrPolicyDomainHcDist_t *) pd)->lockDbLookup);
}

/**
 * @brief Check if the proxy's DB mode is compatible with another DB mode
 * This call helps to determine when an acquire is susceptible to use the proxy DB
 * or if it is not and must handled at a later time.
 *
 * This implementation allows RO over RO and ITW over ITW.
 */
static bool isAcquireEligibleForProxy(ocrDbAccessMode_t proxyDbMode, ocrDbAccessMode_t acquireMode) {
    return (((proxyDbMode == DB_MODE_RO) && (acquireMode == DB_MODE_RO)) ||
     ((proxyDbMode == DB_MODE_ITW) && (acquireMode == DB_MODE_ITW)));
}

/**
 * @brief Enqueue an acquire message into the proxy DB for later processing.
 *
 * Warning: The caller must own the proxy DB internal's lock.
 */
static void enqueueAcquireMessageInProxy(ocrPolicyDomain_t * pd, ProxyDb_t * proxyDb, ocrPolicyMsg_t * msg) {
    // Ensure there's sufficient space in the queue
    if (proxyDb->acquireQueue == NULL) {
        proxyDb->acquireQueue = newBoundedQueue(pd, PROXY_DB_QUEUE_SIZE_DEFAULT);
    }
    if (queueIsFull(proxyDb->acquireQueue)) {
        proxyDb->acquireQueue = queueDoubleResize(proxyDb->acquireQueue, /*freeOld=*/true);
    }
    // Enqueue: Need to make a copy of the message since the acquires are two-way
    // asynchronous and the calling context may disappear later on.
    //DIST-TODO Auto-serialization
    ocrPolicyMsg_t * msgCopy = (ocrPolicyMsg_t *) pd->fcts.pdMalloc(pd, sizeof(ocrPolicyMsg_t));
    hal_memCopy(msgCopy, msg, sizeof(ocrPolicyMsg_t), false);
    queueAddLast(proxyDb->acquireQueue, (void *) msgCopy);
}

/**
 * @brief Dequeue an acquire message from a message queue compatible with a DB access node.
 *
 * @param candidateQueue      The queue to dequeue from
 * @param acquireMode         The DB access mode the message should be compatible with
 *
 * Warning: The caller must own the proxy DB internal's lock.
 */
static Queue_t * dequeueCompatibleAcquireMessageInProxy(ocrPolicyDomain_t * pd, Queue_t * candidateQueue, ocrDbAccessMode_t acquireMode) {
    if ((candidateQueue != NULL) && ((acquireMode == DB_MODE_RO) || (acquireMode == DB_MODE_ITW))) {
        u32 idx = 0;
        Queue_t * eligibleQueue = NULL;
        // Iterate the candidate queue
        while(idx < candidateQueue->tail) {
            ocrPolicyMsg_t * msg = queueGet(candidateQueue, idx);
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
            if ((PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK) == acquireMode) {
                // Found a match
                if (eligibleQueue == NULL) {
                    eligibleQueue = newBoundedQueue(pd, PROXY_DB_QUEUE_SIZE_DEFAULT);
                }
                if (queueIsFull(eligibleQueue)) {
                    eligibleQueue = queueDoubleResize(eligibleQueue, /*freeOld=*/true);
                }
                // Add to eligible queue, remove from candidate queue
                queueAddLast(eligibleQueue, (void *) msg);
                queueRemove(candidateQueue, idx);
            } else {
                idx++;
            }
    #undef PD_MSG
    #undef PD_TYPE
        }
        return eligibleQueue;
    }
    return NULL;
}


/**
 * @brief Update an acquire message with information from a proxy DB
 */
static void updateAcquireMessage(ocrPolicyMsg_t * msg, ProxyDb_t * proxyDb) {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    PD_MSG_FIELD(ptr)  = proxyDb->ptr;
    PD_MSG_FIELD(size) = proxyDb->size;
    PD_MSG_FIELD(properties) = proxyDb->flags;
#undef PD_MSG
#undef PD_TYPE
}

ocrGuid_t hcDistRtEdtRemoteSatisfy(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //Need to send a message to the rank targeted by the localProxy
    //Need to downcast to proxyEvent, retrieve the original dest guid
    //TODO resolve dstGuid from a proxyEvent data-structure or just pass it as paramv ?
    ocrGuid_t dstGuid = (ocrGuid_t) paramv[0];
    // TODO is it always ok depv's guid is the guid the proxyEvent has been satisfied with ?
    ocrGuid_t value = depv[0].guid;
    ocrEventSatisfy(dstGuid, value);
    return NULL_GUID;
}

//DIST-TODO grad/release PD: copy-pasted from hc-policy.c, this stuff need to be refactored
static u8 hcDistGrabPd(ocrPolicyDomainHc_t *rself) {
    START_PROFILE(pd_hc_GrabPd);
    u32 newState = rself->state;
    u32 oldState;
    if((newState & 0xF) == 1) {
        do {
            // Try to grab it
            oldState = newState;
            newState += 16; // Increment the user count by 1, skips the bottom 4 bits
            newState = hal_cmpswap32(&(rself->state), oldState, newState);
            if(newState == oldState) {
                RETURN_PROFILE(0);
            } else {
                if(newState & 0x2) {
                    // The PD is shutting down
                    RETURN_PROFILE(OCR_EAGAIN);
                }
                // Some other thread incremented the reader count so
                // we try again
                ASSERT((newState & 0xF) == 1); // Just to make sure
            }
        } while(true);
    } else {
        RETURN_PROFILE(OCR_EAGAIN);
    }
}

//DIST-TODO grad/release PD: copy-pasted from hc-policy.c, this stuff need to be refactored
static void hcDistReleasePd(ocrPolicyDomainHc_t *rself) {
    START_PROFILE(pd_hc_ReleasePd);
    u32 oldState = 0;
    u32 newState = rself->state;
    do {
        ASSERT(newState > 16); // We must at least be a user
        oldState = newState;
        newState -= 16;
        newState = hal_cmpswap32(&(rself->state), oldState, newState);
    } while(newState != oldState);
    RETURN_PROFILE();
}

u8 resolveRemoteMetaData(ocrPolicyDomain_t * self, ocrFatGuid_t * fGuid, u64 metaDataSize) {
    ocrGuid_t remoteGuid = fGuid->guid;
    u64 val;
    self->guidProviders[0]->fcts.getVal(self->guidProviders[0], remoteGuid, &val, NULL);
    if (val == 0) {
        // GUID is unknown, request a copy of the metadata
        ocrPolicyMsg_t msgClone;
        getCurrentEnv(NULL, NULL, NULL, &msgClone);
        DPRINTF(DEBUG_LVL_VVERB,"Resolving metadata -> need to query remote node (using msg @ 0x%lx)\n", &msgClone);
#define PD_MSG (&msgClone)
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
            msgClone.type = PD_MSG_GUID_METADATA_CLONE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
            PD_MSG_FIELD(guid.guid) = remoteGuid;
            PD_MSG_FIELD(guid.metaDataPtr) = NULL;
            PD_MSG_FIELD(size) = 0ULL; // Prevents serialization of the outgoing request
            u8 returnCode = self->fcts.processMessage(self, &msgClone, true);
            ASSERT(returnCode == 0);
            // On return, Need some more post-processing to make a copy of the metadata
            // and set the fatGuid's metadata ptr to point to the copy
            void * metaDataPtr = self->fcts.pdMalloc(self, metaDataSize);
            ASSERT(PD_MSG_FIELD(guid.metaDataPtr) != NULL);
            ASSERT(PD_MSG_FIELD(guid.guid) == remoteGuid);
            ASSERT(PD_MSG_FIELD(size) == metaDataSize);
            hal_memCopy(metaDataPtr, PD_MSG_FIELD(guid.metaDataPtr), metaDataSize, false);
            //TODO-MSGSIZE sketchy... grep for the other sketchy (1) below in post 2-way unmarshalling code
            self->fcts.pdFree(self, PD_MSG_FIELD(guid.metaDataPtr));
            //DIST-TODO Potentially multiple concurrent registerGuid on the same template
            self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], remoteGuid, (u64) metaDataPtr);
            val = (u64) metaDataPtr;
            DPRINTF(DEBUG_LVL_VVERB,"GUID 0x%lx locally registered @ 0x%lx\n", remoteGuid, val);
#undef PD_MSG
#undef PD_TYPE
    }
    fGuid->metaDataPtr = (void *) val;
    return 0;
}

void getTemplateParamcDepc(ocrPolicyDomain_t * self, ocrFatGuid_t * fatGuid, u32 * paramc, u32 * depc) {
    // Need to deguidify the edtTemplate to know how many elements we're really expecting
    self->guidProviders[0]->fcts.getVal(self->guidProviders[0], fatGuid->guid,
                                        (u64*)&fatGuid->metaDataPtr, NULL);
    ocrTaskTemplate_t * edtTemplate = (ocrTaskTemplate_t *) fatGuid->metaDataPtr;
    if(*paramc == EDT_PARAM_DEF) *paramc = edtTemplate->paramc;
    if(*depc == EDT_PARAM_DEF) *depc = edtTemplate->depc;
}


/*
 * flags : flags of the acquired DB.
 */
static void * acquireLocalDb(ocrPolicyDomain_t * pd, ocrGuid_t dbGuid, ocrDbAccessMode_t mode) {
    ocrTask_t *curTask = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, &curTask, &msg);
    u32 properties = mode | DB_PROP_RT_ACQUIRE;
    if (mode == DB_MODE_NCR) {
        //TODO DBX temporary to avoid interfering with the DB lock protocol
        properties |= DB_PROP_RT_OBLIVIOUS;
    }
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = dbGuid;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(edt.guid) = curTask->guid;
    PD_MSG_FIELD(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD(properties) = properties; // Runtime acquire
    PD_MSG_FIELD(returnDetail) = 0;
    // This call may fail if the policy domain goes down
    // while we are starting to execute
    if(pd->fcts.processMessage(pd, &msg, true)) {
        ASSERT(false); // debug
        return NULL;
    }
    return PD_MSG_FIELD(ptr);
#undef PD_MSG
#undef PD_TYPE
}

// Might be resurrected when we finish NCR DB mode implementation
// static void releaseLocalDb(ocrPolicyDomain_t * pd, ocrGuid_t dbGuid, u64 size) {
//     ocrTask_t *curTask = NULL;
//     ocrPolicyMsg_t msg;
//     getCurrentEnv(NULL, NULL, &curTask, &msg);
// #define PD_MSG (&msg)
// #define PD_TYPE PD_MSG_DB_RELEASE
//     msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
//     PD_MSG_FIELD(guid.guid) = dbGuid;
//     PD_MSG_FIELD(guid.metaDataPtr) = NULL;
//     PD_MSG_FIELD(edt.guid) = curTask->guid;
//     PD_MSG_FIELD(edt.metaDataPtr) = curTask;
//     PD_MSG_FIELD(size) = size;
//     PD_MSG_FIELD(properties) = DB_PROP_RT_ACQUIRE; // Runtime release
//     PD_MSG_FIELD(returnDetail) = 0;
//     // Ignore failures at this point
//     pd->fcts.processMessage(pd, &msg, true);
// #undef PD_MSG
// #undef PD_TYPE
// }

/*
 * Handle messages requiring remote communications, delegate locals to shared memory implementation.
 */
u8 hcDistProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {
    // When isBlocking is false, it means the message processing is FULLY asynchronous.
    // Hence, when processMessage returns it is not guaranteed 'msg' contains the response,
    // even though PD_MSG_REQ_RESPONSE is set.
    // Conversely, when the response is received, the calling context that created 'msg' may
    // not exist anymore. The response policy message must carry all the relevant information
    // so that the PD can process it.

    // This check is only meant to prevent erroneous uses of non-blocking processing for messages
    // that require a response. For now, only PD_MSG_DEP_REGWAITER message is using this feature.
    if ((isBlocking == false) && (msg->type & PD_MSG_REQ_RESPONSE)) {
        ASSERT((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_DB_ACQUIRE);
    }

    //DIST-TODO msg setup: how to double check that: msg->srcLocation has been filled by getCurrentEnv(..., &msg) ?

    // Determine message's recipient and properties:
    // If destination is not set, check if it is a message with an affinity.
    //  If there's an affinity specified:
    //  - Determine an associated location
    //  - Set the msg destination to that location
    //  - Nullify the affinity guid
    // Else assume destination is the current location

    u8 ret = 0;
    ret = hcDistGrabPd((ocrPolicyDomainHc_t*)self);
    if(ret) {
        return ret;
    }
    // Pointer we keep around in case we create a copy original message
    // and need to get back to it
    ocrPolicyMsg_t * originalMsg = msg;

    // Set the actual size of the message as defined per its type if none
    // is provided.
    if (msg->size == 0) {
        // The provided message can be bigger than the msg type size.
        // If no size information has been given we default to that size.
        // Might be wasting a re-allocation later on but at least
        // we are not introducing any errors.
        ocrPolicyMsgGetMsgHeaderSize(msg, &msg->size);
    }

    //DIST-TODO affinity: would help to have a NO_LOC default value
    //The current assumption is that a locally generated message will have
    //src and dest set to the 'current' location. If the message has an affinity
    //hint, it is then used to potentially decide on a different destination.
    ocrLocation_t curLoc = self->myLocation;
    u32 properties = 0;

    // Try to automatically place datablocks and edts
    // Only support naive PD-based placement for now.
    suggestLocationPlacement(self, curLoc, (ocrLocationPlacer_t *) self->placer, msg);

    DPRINTF(DEBUG_LVL_VERB, "HC-dist processing message @ 0x%lx of type 0x%x\n", msg, msg->type);

    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_WORK_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
        DPRINTF(DEBUG_LVL_VVERB,"Processing WORK_CREATE for template GUID 0x%lx\n", PD_MSG_FIELD(templateGuid.guid));
        // First query the guid provider to determine if we know the edtTemplate.
        resolveRemoteMetaData(self, &PD_MSG_FIELD(templateGuid), sizeof(ocrTaskTemplateHc_t));
        // Now that we have the template, we can set paramc and depc correctly
        // This needs to be done because the marshalling of messages relies on paramc and
        // depc being correctly set (so no negative values)
        if((PD_MSG_FIELD(paramc) == EDT_PARAM_DEF) || (PD_MSG_FIELD(depc) == EDT_PARAM_DEF)) {
            getTemplateParamcDepc(self, &PD_MSG_FIELD(templateGuid), &PD_MSG_FIELD(paramc), &PD_MSG_FIELD(depc));
        }
        ASSERT(PD_MSG_FIELD(paramc) != EDT_PARAM_UNK && PD_MSG_FIELD(depc) != EDT_PARAM_UNK);
        if((PD_MSG_FIELD(paramv) == NULL) && (PD_MSG_FIELD(paramc) != 0)) {
            // User error, paramc non zero but no parameters
            DPRINTF(DEBUG_LVL_WARN, "Paramc is non-zero but no paramv was given\n");
            PROCESS_MESSAGE_RETURN_NOW(self, OCR_EINVAL);
        }
        ocrFatGuid_t currentEdt = PD_MSG_FIELD(currentEdt);
        ocrFatGuid_t parentLatch = PD_MSG_FIELD(parentLatch);

        // The placer may have altered msg->destLocation
        if (msg->destLocation == curLoc) {
            DPRINTF(DEBUG_LVL_VVERB,"WORK_CREATE: local EDT creation for template GUID 0x%lx\n", PD_MSG_FIELD(templateGuid.guid));
        } else {
            // Outgoing EDT create message
            DPRINTF(DEBUG_LVL_VVERB,"WORK_CREATE: remote EDT creation at %lu for template GUID 0x%lx\n", (u64)msg->destLocation, PD_MSG_FIELD(templateGuid.guid));
#undef PD_MSG
#undef PD_TYPE
            /* The support for finish EDT and latch in distributed OCR
             * has the following implementation currently:
             * Whenever an EDT needs to be created on a remote node,
             * then a proxy latch is created on the remote node if
             * there is a parent latch to report to on the source node.
             * So, when an parent EDT creates a child EDT on a remote node,
             * the local latch is first incremented on the source node.
             * This latch will eventually be decremented through the proxy
             * latch on the remote node.
             */
            if (parentLatch.guid != NULL_GUID) {
                ocrLocation_t parentLatchLoc;
                RETRIEVE_LOCATION_FROM_GUID(self, parentLatchLoc, parentLatch.guid);
                if (parentLatchLoc == curLoc) {
                    //Check in to parent latch
                    ocrPolicyMsg_t msg2;
                    getCurrentEnv(NULL, NULL, NULL, &msg2);
#define PD_MSG (&msg2)
#define PD_TYPE PD_MSG_DEP_SATISFY
                    // This message MUST be fully processed (i.e. parentLatch satisfied)
                    // before we return. Otherwise there's a race between this registration
                    // and the current EDT finishing.
                    msg2.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
                    PD_MSG_FIELD(guid) = parentLatch;
                    PD_MSG_FIELD(payload.guid) = NULL_GUID;
                    PD_MSG_FIELD(payload.metaDataPtr) = NULL;
                    PD_MSG_FIELD(currentEdt) = currentEdt;
                    PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_INCR_SLOT;
                    PD_MSG_FIELD(properties) = 0;
                    RESULT_PROPAGATE(self->fcts.processMessage(self, &msg2, true));
#undef PD_MSG
#undef PD_TYPE
                } // else, will create a proxy event at current location
            }
        }

        if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
            // we're receiving a message
            DPRINTF(DEBUG_LVL_VVERB, "WORK_CREATE: received request from %d\n", msg->srcLocation);
            if (parentLatch.guid != NULL_GUID) {
                //Create a proxy latch in current node
                /* On the remote side, a proxy latch is first created and a dep
                 * is added to the source latch. Within the remote node, this
                 * proxy latch becomes the new parent latch for the EDT to be
                 * created inside the remote node.
                 */
                ocrPolicyMsg_t msg2;
                getCurrentEnv(NULL, NULL, NULL, &msg2);
#define PD_MSG (&msg2)
#define PD_TYPE PD_MSG_EVT_CREATE
                msg2.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
                PD_MSG_FIELD(guid.guid) = NULL_GUID;
                PD_MSG_FIELD(guid.metaDataPtr) = NULL;
                PD_MSG_FIELD(type) = OCR_EVENT_LATCH_T;
                PD_MSG_FIELD(properties) = 0;
                RESULT_PROPAGATE(self->fcts.processMessage(self, &msg2, true));

                ocrFatGuid_t latchFGuid = PD_MSG_FIELD(guid);
#undef PD_TYPE
#define PD_TYPE PD_MSG_DEP_ADD
                msg2.type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
                PD_MSG_FIELD(source) = latchFGuid;
                PD_MSG_FIELD(dest) = parentLatch;
                PD_MSG_FIELD(slot) = OCR_EVENT_LATCH_DECR_SLOT;
                PD_MSG_FIELD(properties) = DB_MODE_RO; // not called from add-dependence
                RESULT_PROPAGATE(self->fcts.processMessage(self, &msg2, true));
#undef PD_MSG
#undef PD_TYPE

#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
                PD_MSG_FIELD(parentLatch) = latchFGuid;
#undef PD_MSG
#undef PD_TYPE
            }
        }
        break;
    }
    case PD_MSG_DB_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_CREATE
        // The placer may have altered msg->destLocation
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    // Need to determine the destination of the message based
    // on the operation and guids it involves.
    case PD_MSG_WORK_DESTROY:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_WORK_DESTROY
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "WORK_DESTROY: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DEP_SATISFY:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB,"DEP_SATISFY: target is %d\n", (int) msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_EVT_DESTROY:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_EVT_DESTROY
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "EVT_DESTROY: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DB_DESTROY:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_DESTROY
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "DB_DESTROY: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DB_FREE:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_FREE
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "DB_FREE: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_EDTTEMP_DESTROY:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_EDTTEMP_DESTROY
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "EDTTEMP_DESTROY: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_GUID_INFO:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_GUID_INFO
        //DIST-TODO cloning: What's the meaning of guid info in distributed ?
        // TODO PD_MSG_FIELD(guid);
        msg->destLocation = curLoc;
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_GUID_METADATA_CLONE:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "METADATA_CLONE: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_GUID_DESTROY:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "GUID_DESTROY: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_MGT_SHUTDOWN:
    {
        if ((msg->srcLocation == curLoc) && (msg->destLocation == curLoc)) {
            ocrPolicyDomainHcDist_t * rself = ((ocrPolicyDomainHcDist_t*)self);
            DPRINTF(DEBUG_LVL_VERB,"MGT_SHUTDOWN: ocrShutdown invoked\n");
        #define PD_MSG (msg)
        #define PD_TYPE PD_MSG_MGT_SHUTDOWN
            self->shutdownCode = PD_MSG_FIELD(errorCode);
        #undef PD_MSG
        #undef PD_TYPE
            rself->shutdownAckCount++; // Allow quiescence detection code in 'take'
            // return right away
            PROCESS_MESSAGE_RETURN_NOW(self, 0);
        } else if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
            // Incoming shutdown message from another PD
            ocrPolicyDomainHcDist_t * rself = ((ocrPolicyDomainHcDist_t*)self);
            DPRINTF(DEBUG_LVL_VERB,"MGT_SHUTDOWN: incoming shutdown message: ackCount=%lu\n",rself->shutdownAckCount);

            if (rself->shutdownAckCount == 0) {
            #define PD_MSG (msg)
            #define PD_TYPE PD_MSG_MGT_SHUTDOWN
                self->shutdownCode = PD_MSG_FIELD(errorCode);
            #undef PD_MSG
            #undef PD_TYPE
                // If we get a shutdown msg and the current PD count is zero it
                // means it's another PD that got to execute ocrShutdown.
                // Hence incr, to allow quiescence detection code in 'take'
                rself->shutdownAckCount++;
            }
            rself->shutdownAckCount++;
            // let worker '1' do the stop
            PROCESS_MESSAGE_RETURN_NOW(self, 0);
        }
        // Fall-through to send to other PD or local processing.
        break;
    }
    case PD_MSG_DEP_DYNADD:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_DYNADD
        RETRIEVE_LOCATION_FROM_MSG(self, edt, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "DEP_DYNADD: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DEP_DYNREMOVE:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_DYNREMOVE
        RETRIEVE_LOCATION_FROM_MSG(self, edt, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "DEP_DYNREMOVE: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DB_ACQUIRE:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
        if (msg->type & PD_MSG_REQUEST) {
            RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation)
            // Send/Receive to/from remote or local processing, all fall-through
            if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
                // Incoming acquire request
                // The DB MUST be local to this node (that's the acquire is sent to this PD)
                // Need to resolve the DB metadata before handing the message over to the base PD.
                // The sender didn't know about the metadataPtr, receiver does.
                u64 val;
                self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
                ASSERT(val != 0);
                PD_MSG_FIELD(guid.metaDataPtr) = (void *) val;
                DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: Incoming request for DB GUID 0x%lx with properties=0x%x\n",
                        PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
                // Fall-through to local processing
            }
            if ((msg->srcLocation == curLoc) && (msg->destLocation != curLoc)) {
                // Outgoing acquire request
                ProxyDb_t * proxyDb = getProxyDb(self, PD_MSG_FIELD(guid.guid), true);
                hal_lock32(&(proxyDb->lock)); // lock the db
                switch(proxyDb->state) {
                    case PROXY_DB_CREATED:
                        DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: Outgoing request for DB GUID 0x%lx with properties=0x%x, creation fetch\n",
                                PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
                        // The proxy has just been created, need to fetch the DataBlock
                        PD_MSG_FIELD(properties) |= DB_FLAG_RT_FETCH;
                        proxyDb->state = PROXY_DB_FETCH;
                    break;
                    case PROXY_DB_RUN:
                        // The DB is already in use locally
                        // Check if the acquire is compatible with the current usage
                        if (isAcquireEligibleForProxy(proxyDb->mode, (PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK))) {
                            DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: Outgoing request for DB GUID 0x%lx with properties=0x%x, intercepted for local proxy DB\n",
                                    PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
                            //Use the local cached version of the DB
                            updateAcquireMessage(msg, proxyDb);
                            proxyDb->nbUsers++;
                            //Granted access to the DB through the proxy. In that sense the request
                            //has been processed and is now a response to be returned to the caller.
                            msg->type = PD_MSG_RESPONSE;
                            msg->destLocation = curLoc; // optional, doing it to be consistent
                            PD_MSG_FIELD(returnDetail) = 0;
                            // No need to fall-through for local processing, proxy DB is used.
                            hal_unlock32(&(proxyDb->lock));
                            relProxyDb(self, proxyDb);
                            PROCESS_MESSAGE_RETURN_NOW(self, 0);
                        } // else, not eligible to use the proxy, must enqueue the message
                        //WARN: fall-through is intentional
                    case PROXY_DB_FETCH:
                    case PROXY_DB_RELINQUISH:
                        //WARN: Do NOT move implementation: 'PROXY_DB_RUN' falls-through here
                        // The proxy is in a state requiring stalling outgoing acquires.
                        // The acquire 'msg' is copied and enqueued.
                        DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: Outgoing request for DB GUID 0x%lx with properties=0x%x, enqueued\n",
                                PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
                        enqueueAcquireMessageInProxy(self, proxyDb, msg);
                        // Inform caller the acquire is pending.
                        hal_unlock32(&(proxyDb->lock));
                        relProxyDb(self, proxyDb);
                        //NOTE: Important to set return values correctly since the acquire is not done yet !
                        // Here we set the returnDetail of the original message (not the enqueued copy)
                        PD_MSG_FIELD(returnDetail) = OCR_EBUSY;
                        PROCESS_MESSAGE_RETURN_NOW(self, OCR_EPEND);
                    break;
                    default:
                        ASSERT(false && "Unsupported proxy DB state");
                    // Fall-through to send the outgoing message
                }
                hal_unlock32(&(proxyDb->lock));
                relProxyDb(self, proxyDb);
            }
        } else { // DB_ACQUIRE response
            ASSERT(msg->type & PD_MSG_RESPONSE);
            RETRIEVE_LOCATION_FROM_MSG(self, edt, msg->destLocation)
            if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
                // Incoming acquire response
                ProxyDb_t * proxyDb = getProxyDb(self, PD_MSG_FIELD(guid.guid), false);
                // Cannot receive a response to an acquire if we don't have a proxy
                ASSERT(proxyDb != NULL);
                hal_lock32(&(proxyDb->lock)); // lock the db
                DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: Incoming response for DB GUID 0x%lx with properties=0x%x\n",
                        PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
                switch(proxyDb->state) {
                    case PROXY_DB_FETCH:
                        // Processing an acquire response issued in the fetch state
                        // Update message properties
                        PD_MSG_FIELD(properties) &= ~DB_FLAG_RT_FETCH;
                        //TODO double check but I think we don't need the WB flag anymore since we have the mode
                        bool doWriteBack = !((PD_MSG_FIELD(properties) & DB_MODE_NCR) || (PD_MSG_FIELD(properties) & DB_MODE_RO) ||
                                             (PD_MSG_FIELD(properties) & DB_PROP_SINGLE_ASSIGNMENT));
                        if (doWriteBack) {
                            PD_MSG_FIELD(properties) |= DB_FLAG_RT_WRITE_BACK;
                        }
                        // Try to double check that across acquires the DB size do not change
                        ASSERT((proxyDb->size != 0) ? (proxyDb->size == PD_MSG_FIELD(size)) : true);

                        DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: caching data copy for DB GUID 0x%lx msgSize=%lu \n",
                            PD_MSG_FIELD(guid.guid), msg->size);

                        // update the proxy DB
                        ASSERT(proxyDb->nbUsers == 0);
                        proxyDb->nbUsers++; // checks in as a proxy user
                        proxyDb->state = PROXY_DB_RUN;
                        proxyDb->mode = (PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK);
                        proxyDb->size = PD_MSG_FIELD(size);
                        proxyDb->flags = PD_MSG_FIELD(properties);
                        // Deserialize the data pointer from the message
                        // The message ptr is set to the message payload but we need
                        // to make a copy since the message will be deallocated later on.
                        void * newPtr = proxyDb->ptr; // See if we can reuse the old pointer
                        if (newPtr == NULL) {
                            newPtr = self->fcts.pdMalloc(self, proxyDb->size);
                        }
                        void * msgPayloadPtr = PD_MSG_FIELD(ptr);
                        hal_memCopy(newPtr, msgPayloadPtr, proxyDb->size, false);
                        proxyDb->ptr = newPtr;
                        // Update message to be consistent, but no calling context should need to read it.
                        PD_MSG_FIELD(ptr) = proxyDb->ptr;
                        DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: caching data copy for DB GUID 0x%lx ptr=%p size=%lu flags=0x%x\n",
                            PD_MSG_FIELD(guid.guid), proxyDb->ptr, proxyDb->size, proxyDb->flags);
                        // Scan queue for compatible acquire that could use this cached proxy DB
                        Queue_t * eligibleQueue = dequeueCompatibleAcquireMessageInProxy(self, proxyDb->acquireQueue, proxyDb->mode);

                        // Iterate the queue and process pending acquire messages:
                        // Now the proxy state is RUN. All calls to process messages for
                        // eligible acquires will succeed in getting and using the cached data.
                        // Also note that the current acquire being checked in (proxy's count is one)
                        // ensures the proxy stays in RUN state when the current worker will
                        // fall-through to local processing.

                        if (eligibleQueue != NULL) {
                            u32 idx = 0;
                            // Update the proxy DB counter once (all subsequent acquire MUST be successful or there's a bug)
                            proxyDb->nbUsers += queueGetSize(eligibleQueue);
                            while(idx < queueGetSize(eligibleQueue)) {
                                ocrPolicyMsg_t * msg = (ocrPolicyMsg_t *) queueGet(eligibleQueue, idx);
                                //NOTE: if we were to cache the proxyDb info we could release the proxy
                                //lock before this loop and allow for more concurrency. Although we would
                                //not have a pointer to the proxy, we would have one to the DB ptr data.
                                //I'm not sure if that means we're breaking the refCount abstraction.
                                updateAcquireMessage(msg, proxyDb);
                                DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: dequeued eligible acquire for DB GUID 0x%lx with properties=0x%x\n",
                                    PD_MSG_FIELD(guid.guid), proxyDb->flags);
                                // The acquire message had been processed once and was enqueued.
                                // Now it is processed 'again' but immediately succeeds in acquiring
                                // the cached data from the proxy and potentially iterates the acquire
                                // frontier of the EDT that originally called acquire.

                                // For the frontier to be iterated we need to directly call the base implementation
                                // and treat this request as a response.
                                msg->type &= ~PD_MSG_REQUEST;
                                msg->type &= ~PD_MSG_REQ_RESPONSE;
                                msg->type |= PD_MSG_RESPONSE;
                                msg->srcLocation = curLoc;
                                msg->destLocation = curLoc;
                                ocrPolicyDomainHcDist_t * pdSelfDist = (ocrPolicyDomainHcDist_t *) self;
                                // This call MUST succeed or there's a bug in the implementation.
                                u8 returnCode = pdSelfDist->baseProcessMessage(self, msg, false);
                                ASSERT(PD_MSG_FIELD(returnDetail) == 0); // Message's processing return code
                                ASSERT(returnCode == 0); // processMessage return code
                                // Free the message (had been copied when enqueued)
                                self->fcts.pdFree(self, msg);
                                idx++;
                            }
                            queueDestroy(eligibleQueue);
                        }
                        hal_unlock32(&(proxyDb->lock));
                        relProxyDb(self, proxyDb);
                        // Fall-through to local processing:
                        // This acquire may be part of an acquire frontier that needs to be iterated over
                    break;
                    // Handle all the invalid cases
                    case PROXY_DB_CREATED:
                        // Error in created state: By design cannot receive an acquire response in this state
                        ASSERT(false && "Invalid proxy DB state: PROXY_DB_CREATED processing an acquire response");
                    break;
                    case PROXY_DB_RUN:
                        // Error in run state: an acquire is either local and use the cache copy or is enqueued
                        ASSERT(false && "Invalid proxy DB state: PROXY_DB_RUN processing an acquire response");
                    break;
                    case PROXY_DB_RELINQUISH:
                        // Error in relinquish state: all should have been enqueued
                        ASSERT(false && "Invalid proxy DB state: PROXY_DB_RELINQUISH processing an acquire response");
                    break;
                    default:
                        ASSERT(false && "Unsupported proxy DB state");
                    // Fall-through to process the incoming acquire response
                }
                // processDbAcquireResponse(self, msg);
            } // else outgoing acquire response to be sent out, fall-through
        }
        if ((msg->srcLocation == curLoc) && (msg->destLocation == curLoc)) {
            DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: local request for DB GUID 0x%lx with properties 0x%x\n",
                    PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
        }
        // Let the base policy's processMessage acquire the DB on behalf of the remote EDT
        // and then append the db data to the message.
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DB_RELEASE:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_RELEASE
        RETRIEVE_LOCATION_FROM_GUID_MSG(self, msg->destLocation)
        if ((msg->srcLocation == curLoc) && (msg->destLocation != curLoc)) {
            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN outgoing request send for DB GUID 0x%lx\n", PD_MSG_FIELD(guid.guid));
            // Outgoing release request
            ProxyDb_t * proxyDb = getProxyDb(self, PD_MSG_FIELD(guid.guid), false);
            hal_lock32(&(proxyDb->lock)); // lock the db
            switch(proxyDb->state) {
                case PROXY_DB_RUN:
                    if (proxyDb->nbUsers == 1) {
                        // Last checked-in user of the proxy DB in this PD
                        proxyDb->state = PROXY_DB_RELINQUISH;
                        DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN outgoing request send for DB GUID 0x%lx with WB=%d\n",
                            PD_MSG_FIELD(guid.guid), !!(proxyDb->flags & DB_FLAG_RT_WRITE_BACK));
                        if (proxyDb->flags & DB_FLAG_RT_WRITE_BACK) {
                            // Serialize the cached DB ptr for write back
                            u64 dbSize = proxyDb->size;
                            void * dbPtr = proxyDb->ptr;
                            // Update the message's properties
                            PD_MSG_FIELD(properties) |= DB_FLAG_RT_WRITE_BACK;
                            PD_MSG_FIELD(ptr) = dbPtr;
                            PD_MSG_FIELD(size) = dbSize;
                            //ptr is updated on the other end when deserializing
                        } else {
                            // Just to double check if we missed callsites
                            ASSERT(PD_MSG_FIELD(ptr) == NULL);
                            ASSERT(PD_MSG_FIELD(size) == 0);
                            // no WB, make sure these are set to avoid erroneous serialization
                            PD_MSG_FIELD(ptr) = NULL;
                            PD_MSG_FIELD(size) = 0;
                        }
                        // Fall-through and send the release request
                        // The count is decremented when the release response is received
                    } else {
                        DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN outgoing request send for DB GUID 0x%lx intercepted for local proxy DB\n",
                            PD_MSG_FIELD(guid.guid), !!(proxyDb->flags & DB_FLAG_RT_WRITE_BACK));
                        // The proxy DB is still in use locally, no need to notify the original DB.
                        proxyDb->nbUsers--;
                        // fill in response message
                        msg->type &= ~PD_MSG_REQUEST;
                        msg->type &= ~PD_MSG_REQ_RESPONSE;
                        msg->type |= PD_MSG_RESPONSE;
                        msg->srcLocation = curLoc;
                        msg->destLocation = curLoc;
                        PD_MSG_FIELD(returnDetail) = 0;
                        hal_unlock32(&(proxyDb->lock));
                        relProxyDb(self, proxyDb);
                        PROCESS_MESSAGE_RETURN_NOW(self, 0); // bypass local processing
                    }
                break;
                // Handle all the invalid cases
                case PROXY_DB_CREATED:
                    // Error in created state: By design cannot release before acquire
                    ASSERT(false && "Invalid proxy DB state: PROXY_DB_CREATED processing a release request");
                break;
                case PROXY_DB_FETCH:
                    // Error in run state: Cannot release before initial acquire has completed
                    ASSERT(false && "Invalid proxy DB state: PROXY_DB_RUN processing a release request");
                break;
                case PROXY_DB_RELINQUISH:
                    // Error in relinquish state: By design the last release transitions the proxy from run to
                    // relinquish. An outgoing release request while in relinquish state breaks this invariant.
                    ASSERT(false && "Invalid proxy DB state: PROXY_DB_RELINQUISH processing a release request");
                break;
                default:
                    ASSERT(false && "Unsupported proxy DB state");
                // Fall-through to send the outgoing message
            }
            hal_unlock32(&(proxyDb->lock));
            relProxyDb(self, proxyDb);
        }

        if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
            // Incoming DB_RELEASE pre-processing
            // Need to resolve the DB metadata locally before handing the message over
            u64 val;
            self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
            ASSERT(val != 0);
            PD_MSG_FIELD(guid.metaDataPtr) = (void *) val;
            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN incoming request received for DB GUID 0x%lx WB=%d\n",
                    PD_MSG_FIELD(guid.guid), !!(PD_MSG_FIELD(properties) & DB_FLAG_RT_WRITE_BACK));
            //DIST-TODO db: We may want to double check this writeback (first one) is legal wrt single assignment
            if (PD_MSG_FIELD(properties) & DB_FLAG_RT_WRITE_BACK) {
                // Unmarshall and write back
                //WARN: MUST read from the RELEASE size u64 field instead of the msg size (u32)
                u64 size = PD_MSG_FIELD(size);
                // void * data = findMsgPayload(msg, NULL);
                void * data = PD_MSG_FIELD(ptr);
                // Acquire local DB on behalf of remote release to do the writeback.
                // Do it in OB mode so as to make sure the acquire goes through
                // We perform the write-back and then fall-through for the actual db release.
                //TODO DBX DB_MODE_NCR is not implemented, we're converting the call to a special
                void * localData = acquireLocalDb(self, PD_MSG_FIELD(guid.guid), DB_MODE_NCR);
                ASSERT(localData != NULL);
                hal_memCopy(localData, data, size, false);
                // TODO DBX We do not release here because we've been using this special mode to do the write back
                // releaseLocalDb(self, PD_MSG_FIELD(guid.guid), size);
            } // else fall-through and do the regular release
        }
        if ((msg->srcLocation == curLoc) && (msg->destLocation == curLoc)) {
            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN local processing: DB GUID 0x%lx\n", PD_MSG_FIELD(guid.guid));
        }
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DEP_REGSIGNALER:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_REGSIGNALER
        RETRIEVE_LOCATION_FROM_MSG(self, dest, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "DEP_REGSIGNALER: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_DEP_REGWAITER:
    {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DEP_REGWAITER
        RETRIEVE_LOCATION_FROM_MSG(self, dest, msg->destLocation);
        DPRINTF(DEBUG_LVL_VVERB, "DEP_REGWAITER: target is %d\n", (int)msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    case PD_MSG_COMM_TAKE: {
        ocrPolicyDomainHcDist_t* rself = (ocrPolicyDomainHcDist_t*)self;
        if (rself->shutdownAckCount > 0) {
            ocrWorker_t * worker;
            getCurrentEnv(NULL, &worker, NULL, NULL);
            //DIST-TODO sep-concern: The PD should not know about underlying worker impl
            ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
            int wid = hcWorker->id;
            if ((wid == 1) && (rself->piledWorkerCtx == 0)) {
                // Shutdown was called and there's no helper-mode in progress
                // Check if other comp-workers are busy
                DPRINTF(DEBUG_LVL_VVERB,"MGT_SHUTDOWN: wait for comp-worker to wrap-up before shutdown %d\n");
                u32 i = 0;
                while(i < self->workerCount) {
                    if ((hcWorker->hcType == HC_WORKER_COMP) && (self->workers[i]->curTask != NULL)) {
                        break; // still some busy worker, will fall-through to do the regular take.
                    }
                    i++;
                }
                //Note: there's still a potential race when a RT EDT is in the deque but we failed to see it
                if ((i == self->workerCount) && (!rself->shutdownSent)) {
                    // Everybody was quiet, there might be in-flight wrap up messages (dbRelease and so in epilogue EDTs)
                    // still attempt to notify all other PDs, they should be denied PD access thanks to grabLock
                    //DIST-TODO These should be sent in parallel
                    i = 0;
                    while(i < self->neighborCount) {
                        DPRINTF(DEBUG_LVL_VVERB,"MGT_SHUTDOWN: loop shutdown neighbors[%d] is %d\n", i, (int) self->neighbors[i]);
                        ocrPolicyMsg_t msgShutdown;
                        getCurrentEnv(NULL, NULL, NULL, &msgShutdown);
                    #define PD_MSG (&msgShutdown)
                    #define PD_TYPE PD_MSG_MGT_SHUTDOWN
                        msgShutdown.destLocation = self->neighbors[i];
                        PD_MSG_FIELD(errorCode) = self->shutdownCode;
                        DPRINTF(DEBUG_LVL_VERB,"MGT_SHUTDOWN: send shutdown msg to %d\n", (int) msgShutdown.destLocation);
                        // Shutdown is a two-way message. It gives the target the opportunity to drain and
                        // finalize some of its pending communication (think dbRelease called after the EDT
                        // that triggered shutdown on another node)
                        msgShutdown.type = PD_MSG_MGT_SHUTDOWN | PD_MSG_REQUEST;
                        u8 returnCode = self->fcts.processMessage(self, &msgShutdown, true);
                        ASSERT(returnCode == 0);
                    #undef PD_MSG
                    #undef PD_TYPE
                        i++;
                    }
                    rself->shutdownSent = true;
                }
                if (rself->shutdownSent && (rself->shutdownAckCount == (self->neighborCount+1))) {
                    //Note: there's still a potential race when a RT EDT is in the deque but we failed to see it
                    // We need to do this again because when we receive shutdown ack as soon as shutdownAckCount
                    // is incremented we can come here although the EDT that did the increment is still running
                    i = 0;
                    while(i < self->workerCount) {
                        if ((hcWorker->hcType == HC_WORKER_COMP) && (self->workers[i]->curTask != NULL)) {
                            break; // still some busy worker, will fall-through to do the regular take.
                        }
                        i++;
                    }
                    if (i == self->workerCount) {
                        DPRINTF(DEBUG_LVL_VERB,"MGT_SHUTDOWN: received all shutdown\n");
                    #define PD_MSG (msg)
                    #define PD_TYPE PD_MSG_COMM_TAKE
                        PD_MSG_FIELD(guidCount) = 0;
                        msg->type &= ~PD_MSG_REQUEST;
                        msg->type |= PD_MSG_RESPONSE;
                    #undef PD_MSG
                    #undef PD_TYPE
                        // Received all the shutdown, we stop
                        //NOTE: We shutdown here otherwise we need to change the state
                        //to spin on from 18 to 34 in hcPolicyDomainStop because the double
                        //acquisition caused by calling the base impl.
                        //Shutdown are one way messages in distributed. Avoids having to
                        //send a response when the comms are shutting down.
                        self->fcts.stop(self);
                        PROCESS_MESSAGE_RETURN_NOW(self, 0);
                    } // else should only go for another round until the EDT that did the ack increment is alive
               }
            }
        } // fall-through and do regular take
        break;
    }
    case PD_MSG_MGT_MONITOR_PROGRESS:
    {
        msg->destLocation = curLoc;
        // This is to temporarily help with the shutdown process see issue #200
        hal_lock32(&((ocrPolicyDomainHcDist_t*)self)->piledWorkerCtxLock);
        ((ocrPolicyDomainHcDist_t*)self)->piledWorkerCtx++;
        hal_unlock32(&((ocrPolicyDomainHcDist_t*)self)->piledWorkerCtxLock);
        // fall-through and let local PD to process
        break;
    }
    // filter out local messages
    case PD_MSG_DEP_ADD:
    case PD_MSG_MEM_OP:
    case PD_MSG_MEM_ALLOC:
    case PD_MSG_MEM_UNALLOC:
    case PD_MSG_WORK_EXECUTE:
    case PD_MSG_EDTTEMP_CREATE:
    case PD_MSG_EVT_CREATE:
    case PD_MSG_EVT_GET:
    case PD_MSG_GUID_CREATE:
    case PD_MSG_COMM_GIVE:
    case PD_MSG_DEP_UNREGSIGNALER:
        //DIST-TODO not-supported: I don't think PD_MSG_DEP_UNREGSIGNALER is supported yet even in shared-memory
    case PD_MSG_DEP_UNREGWAITER:
        //DIST-TODO not-supported: I don't think PD_MSG_DEP_UNREGWAITER is support yet even in shared-memory
    case PD_MSG_SAL_OP:
    case PD_MSG_SAL_PRINT:
    case PD_MSG_SAL_READ:
    case PD_MSG_SAL_WRITE:
    case PD_MSG_SAL_TERMINATE:
    case PD_MSG_MGT_OP: //DIST-TODO not-supported: PD_MSG_MGT_OP is probably not always local
    case PD_MSG_MGT_FINISH:
    case PD_MSG_MGT_REGISTER:
    case PD_MSG_MGT_UNREGISTER:
    {
        msg->destLocation = curLoc;
        // for all local messages, fall-through and let local PD to process
        break;
    }
    default:
        //DIST-TODO not-supported: not sure what to do with those.
        // ocrDbReleaseocrDbMalloc, ocrDbMallocOffset, ocrDbFree, ocrDbFreeOffset

        // This is just a fail-safe to make sure the
        // PD impl accounts for all type of messages.
        ASSERT(false && "Unsupported message type");
    }

    // By now, we must have decided what's the actual destination of the message

    // Delegate msg to another PD
    if(msg->destLocation != curLoc) {
        //NOTE: Some of the messages logically require a response, but the PD can
        // already know what to return or can generate a response on behalf
        // of another PD and let it know after the fact. In that case, the PD may
        // void the PD_MSG_REQ_RESPONSE msg's type and treat the call as a one-way

        // Message requires a response, send request and wait for response.
        if ((msg->type & PD_MSG_REQ_RESPONSE) && isBlocking) {
            DPRINTF(DEBUG_LVL_VVERB,"Can't process message locally sending and "
                    "processing a two-way message @ (orig: 0x%lx, now: 0x%lx) to %d\n", originalMsg, msg,
                    (int)msg->destLocation);
            // Since it's a two-way, we'll be waiting for the response and set PERSIST.
            // NOTE: underlying comm-layer may or may not make a copy of msg.
            properties |= TWOWAY_MSG_PROP;
            properties |= PERSIST_MSG_PROP;
            ocrMsgHandle_t * handle = NULL;

            self->fcts.sendMessage(self, msg->destLocation, msg, &handle, properties);
            // Wait on the response handle for the communication to complete.
            DPRINTF(DEBUG_LVL_VVERB,"Waiting for reply from %d\n", (int)msg->destLocation);
            self->fcts.waitMessage(self, &handle);
            DPRINTF(DEBUG_LVL_VVERB,"Received reply from %d for original message @ 0x%lx\n",
                    (int)msg->destLocation, originalMsg);
            ASSERT(handle->response != NULL);

            // Check if we need to copy the response header over to the request msg.
            // Happens when the message includes some additional variable size payload
            // and request message cannot be reused. Or the underlying communication
            // platform was not able to reuse the request message buffer.

            //
            // Warning: From now on EXCLUSIVELY work on the response message
            //

            ocrPolicyMsg_t * response = handle->response;
            DPRINTF(DEBUG_LVL_VERB, "Processing response @ 0x%lx to original message @ 0x%lx\n", response, originalMsg);
            switch (response->type & PD_MSG_TYPE_ONLY) {
            case PD_MSG_DB_ACQUIRE:
            {
                // Shouldn't happen in this implementation.
                ASSERT(false && "Unhandled blocking acquire message");
            break;
            }
            case PD_MSG_DB_CREATE:
            {
                // Receiving the reply for a DB_CREATE
                // Because the current DB creation implementation does not issue a full-fleshed
                // acquire message but rather only do a local acquire at destination, we need
                // to create and populate the proxy here and replicate what a new acquire on a
                // not proxied DB would have been doing
#define PD_MSG (response)
#define PD_TYPE PD_MSG_DB_CREATE
                ocrGuid_t dbGuid = PD_MSG_FIELD(guid.guid);
                ProxyDb_t * proxyDb = createProxyDb(self);
                proxyDb->state = PROXY_DB_RUN;
                proxyDb->nbUsers = 1; // self
                proxyDb->refCount = 0; // ref hasn't been shared yet so '0' is fine.
                proxyDb->mode = (PD_MSG_FIELD(properties) & DB_PROP_MODE_MASK);
                if (proxyDb->mode == 0) {
                    printf("WARNING: DB create mode not found !!!, default to ITW\n");
                    proxyDb->mode = DB_MODE_ITW;
                }
                proxyDb->size = PD_MSG_FIELD(size);
                proxyDb->ptr =  self->fcts.pdMalloc(self, PD_MSG_FIELD(size));
                // Preset the writeback flag: even single assignment needs to be written back the first time.
                proxyDb->flags = (PD_MSG_FIELD(properties) | DB_FLAG_RT_WRITE_BACK);
                // double check there's no proxy registered for the same DB
                u64 val;
                self->guidProviders[0]->fcts.getVal(self->guidProviders[0], dbGuid, &val, NULL);
                ASSERT(val == 0);
                // Do the actual registration
                self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], dbGuid, (u64) proxyDb);
                // Update message with proxy DB ptr
                PD_MSG_FIELD(ptr) = proxyDb->ptr;
#undef PD_MSG
#undef PD_TYPE
            break;
            }
            case PD_MSG_DB_RELEASE:
            {
#define PD_MSG (response)
#define PD_TYPE PD_MSG_DB_RELEASE
                ProxyDb_t * proxyDb = getProxyDb(self, PD_MSG_FIELD(guid.guid), false);
                ASSERT(proxyDb != NULL);
                hal_lock32(&(proxyDb->lock)); // lock the db
                switch(proxyDb->state) {
                case PROXY_DB_RELINQUISH:
                    // Should have a single user since relinquish state
                    // forces concurrent acquires to be queued.
                    ASSERT(proxyDb->nbUsers == 1);
                    // The release having occurred, the proxy's metadata is invalid.
                    if (queueIsEmpty(proxyDb->acquireQueue)) {
                        // There are no pending acquire for this DB, try to deallocate the proxy.
                        hal_lock32(&((ocrPolicyDomainHcDist_t *) self)->lockDbLookup);
                        // Here nobody else can acquire a reference on the proxy
                        if (proxyDb->refCount == 1) {
                            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN response received for DB GUID 0x%lx, destroy proxy\n", PD_MSG_FIELD(guid.guid));
                            // Nullify the entry for the proxy DB in the GUID provider
                            self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], PD_MSG_FIELD(guid.guid), (u64) 0);
                            // Nobody else can get a reference on the proxy's lock now
                            hal_unlock32(&((ocrPolicyDomainHcDist_t *) self)->lockDbLookup);
                            // Deallocate the proxy DB and the cached ptr
                            // NOTE: we do not unlock proxyDb->lock not call relProxyDb
                            // since we're destroying the whole proxy and we're the last user.
                            self->fcts.pdFree(self, proxyDb->ptr);
                            if (proxyDb->acquireQueue != NULL) {
                                self->fcts.pdFree(self, proxyDb->acquireQueue);
                            }
                            self->fcts.pdFree(self, proxyDb);
                        } else {
                            // Not deallocating the proxy then allow others to grab a reference
                            hal_unlock32(&((ocrPolicyDomainHcDist_t *) self)->lockDbLookup);
                            // Else no pending acquire enqueued but someone already got a reference
                            // to the proxyDb, repurpose the proxy for a new fetch
                            // Resetting the state to created means the any concurrent acquire
                            // to the currently executing call will try to fetch the DB.
                            resetProxyDb(proxyDb);
                            // Allow others to use the proxy
                            hal_unlock32(&(proxyDb->lock));
                            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN response received for DB GUID 0x%lx, proxy is referenced\n", PD_MSG_FIELD(guid.guid));
                            relProxyDb(self, proxyDb);
                        }
                    } else {
                        // else there are pending acquire, repurpose the proxy for a new fetch
                        // Resetting the state to created means the popped acquire or any concurrent
                        // acquire to the currently executing call will try to fetch the DB.
                        resetProxyDb(proxyDb);
                        DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE_WARN response received for DB GUID 0x%lx, processing queued acquire\n", PD_MSG_FIELD(guid.guid));
                        // DBs are not supposed to be resizable hence, do NOT reset
                        // size and ptr so they can be reused in the subsequent fetch.
                        // NOTE: There's a size check when an acquire fetch completes and we reuse the proxy.
                        // Pop one of the enqueued acquire and process it 'again'.
                        ocrPolicyMsg_t * pendingAcquireMsg = (ocrPolicyMsg_t *) queueRemoveLast(proxyDb->acquireQueue);
                        hal_unlock32(&(proxyDb->lock));
                        relProxyDb(self, proxyDb);
                        // Now this processMessage call is potentially concurrent with new acquire being issued
                        // by other EDTs on this node. It's ok, either this call succeeds or the acquire is enqueued again.
                        u8 returnCode = self->fcts.processMessage(self, pendingAcquireMsg, false);
                        ASSERT((returnCode == 0) || (returnCode == OCR_EPEND));
                        if (returnCode == OCR_EPEND) {
                            // If the acquire didn't succeed, the message has been copied and enqueued
                            self->fcts.pdFree(self, pendingAcquireMsg);
                        }
                    }
                break;
                // Handle all the invalid cases
                case PROXY_DB_CREATED:
                    // Error in created state: By design cannot release before acquire
                    ASSERT(false && "Invalid proxy DB state: PROXY_DB_CREATED processing a release response");
                break;
                case PROXY_DB_FETCH:
                    // Error in run state: Cannot release before initial acquire has completed
                    ASSERT(false && "Invalid proxy DB state: PROXY_DB_RUN processing a release response");
                break;
                case PROXY_DB_RUN:
                    // Error the acquire request should have transitioned the proxy from the run state to the
                    // relinquish state
                    ASSERT(false && "Invalid proxy DB state: PROXY_DB_RELINQUISH processing a release response");
                break;
                default:
                    ASSERT(false && "Unsupported proxy DB state");
                }
#undef PD_MSG
#undef PD_TYPE
            break;
            }
            default: {
                break;
            }
            } //end switch

            //
            // At this point the response message is ready to be returned
            //

            // Since the caller only has access to the original message we need
            // to make sure it's up-to-date.

            //TODO-MSGSIZE: even if original contains the response that has been
            // unmarshalled there, how safe it is to let the message's payload
            // pointers escape into the wild ? They become invalid when the message
            // is deleted.

            if (originalMsg != handle->response) {
                //TODO-MSGSIZE Here there are a few issues:
                // - The response message may include marshalled pointers, hence
                //   the original message may be too small to accomodate the payload part.
                //   In that case, I don't see how to avoid malloc-ing new memory for each
                //   pointer and update the originalMsg's members, since the use of that
                //   pointers may outlive the message lifespan. Then there's the question
                //   of when those are freed.

                // Calling 'ocrPolicyMsgGetMsgSize', resets the message size to the header size,
                // so that marshall 'duplicate' only copies the header part first
                u64 respFullMsgSize = 0, respMarshalledSize = 0;
                ocrPolicyMsgGetMsgSize(handle->response, &respFullMsgSize, &respMarshalledSize, MARSHALL_DBPTR);
                if (respFullMsgSize > originalMsg->size) {
                    // Do something for pointers
                    switch(originalMsg->type & PD_MSG_TYPE_ONLY) {
                    case PD_MSG_GUID_METADATA_CLONE:
                        {
                            // Copy the header from 'response' to 'originalMsg'
                            hal_memCopy(originalMsg, handle->response, handle->response->size, false);
#define PD_MSG (handle->response)
#define PD_TYPE PD_MSG_GUID_METADATA_CLONE
                            //TODO-MSGSIZE this is kind of sketchy because now the caller
                            //needs to deallocate this at some point.
                            void * metaDataPtr = self->fcts.pdMalloc(self, PD_MSG_FIELD(size));
                            hal_memCopy(metaDataPtr, PD_MSG_FIELD(guid.metaDataPtr), PD_MSG_FIELD(size), false);
#undef PD_MSG
#define PD_MSG (originalMsg)
                            PD_MSG_FIELD(guid.metaDataPtr) = metaDataPtr;
#undef PD_MSG
#undef PD_TYPE
                        break;
                        }
                    case PD_MSG_WORK_CREATE:
                        {
                            ASSERT(false && "PD_MSG_WORK_CREATE Unhandled unmarshalling copy");
                            //TODO write utility functions to copy 'in' or 'out' from one message to another
                            // Copy all 'in/out' and 'out' PD_MSG_WORK_CREATE members from the response
                            // to the request
#define PD_TYPE PD_MSG_WORK_CREATE
#define PD_MSG (handle->response)
                            ocrFatGuid_t guid = PD_MSG_FIELD(guid);
                            ocrFatGuid_t outputEvent = PD_MSG_FIELD(outputEvent);
                            u32 paramc = PD_MSG_FIELD(paramc);
                            u32 depc = PD_MSG_FIELD(depc);
                            u32 returnDetail = PD_MSG_FIELD(returnDetail);
#undef PD_MSG
#define PD_MSG (originalMsg)
                            PD_MSG_FIELD(guid) = guid;
                            PD_MSG_FIELD(outputEvent) = outputEvent;
                            PD_MSG_FIELD(paramc) = paramc;
                            PD_MSG_FIELD(depc) = depc;
                            PD_MSG_FIELD(returnDetail) = returnDetail;
#undef PD_MSG
#undef PD_TYPE
                        break;
                        }
                    case PD_MSG_DB_RELEASE:
                        {
#define PD_TYPE PD_MSG_DB_RELEASE
#define PD_MSG (handle->response)
                            ASSERT(false && "PD_MSG_DB_RELEASE Unhandled unmarshalling copy");
                            u32 returnDetail = PD_MSG_FIELD(returnDetail);
#undef PD_MSG
#define PD_MSG (originalMsg)
                            PD_MSG_FIELD(returnDetail) = returnDetail;
#undef PD_MSG
#undef PD_TYPE
                        break;
                        }
                        default:
                            ASSERT(false && "Unhandled unmarshalling copy");
                    }
                } else {
                    ASSERT(handle->response->size <= originalMsg->size);
                    //TODO-MSGSIZE: This is basically, ignoring the response payload but
                    // it does marshall the originalMsg to itself. i.e. providing it is large
                    // enough, each current pointer is copied at the end of the message as payload,
                    // and the pointers then points to that data.
                    // Marshall 'response' into 'originalMsg' by duplicating the payload
                    ocrPolicyMsgMarshallMsg(handle->response, (u8*)originalMsg, MARSHALL_DUPLICATE | MARSHALL_DBPTR);
                    // originalMsg's size is now the full size.
                }
                self->fcts.pdFree(self, handle->response);
            } else {
                if (msg != handle->response) {
                    // Response is in a different message, copy and free
                    u64 respFullMsgSize = 0, respMarshalledSize = 0;
                    ocrPolicyMsgGetMsgSize(handle->response, &respFullMsgSize, &respMarshalledSize, MARSHALL_DBPTR);
                    // For now
                    ASSERT(respFullMsgSize <= sizeof(ocrPolicyMsg_t));
                    ocrPolicyMsgMarshallMsg(handle->response, (u8*)originalMsg, MARSHALL_DUPLICATE | MARSHALL_DBPTR);
                    self->fcts.pdFree(self, handle->response);
                }
            }
            handle->destruct(handle);
        } else {
            // Either a one-way request or an asynchronous two-way
            DPRINTF(DEBUG_LVL_VVERB,"Sending a one-way request or response to asynchronous two-way msg @ 0x%lx to %d\n",
                    msg, (int) msg->destLocation);

            if (msg->type & PD_MSG_REQ_RESPONSE) {
                ret = OCR_EPEND; // return to upper layer the two-way is pending
            }

            //LIMITATION: For one-way we cannot use PERSIST and thus must request
            // a copy to be done because current implementation doesn't support
            // "waiting" on a one-way.
            u32 sendProp = (msg->type & PD_MSG_REQ_RESPONSE) ? ASYNC_MSG_PROP : 0; // indicates copy required

            if (msg->type & PD_MSG_RESPONSE) {
                sendProp = ASYNC_MSG_PROP;
                // Outgoing asynchronous response for a two-way communication
                // Should be the only asynchronous two-way msg kind for now
                ASSERT((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_DB_ACQUIRE);
                // Marshall outgoing DB_ACQUIRE Response
                switch(msg->type & PD_MSG_TYPE_ONLY) {
                case PD_MSG_DB_ACQUIRE:
                {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
                    DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: Outgoing response for DB GUID 0x%lx with properties=0x%x and dest=%d\n",
                            PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties), (int) msg->destLocation);
#undef PD_MSG
#undef PD_TYPE
                break;
                }
                default: { }
                }
            }

            // one-way request, several options:
            // - make a copy in sendMessage (current strategy)
            // - submit the message to be sent and wait for delivery
            self->fcts.sendMessage(self, msg->destLocation, msg, NULL, sendProp);
        }
    } else {
        // Local PD handles the message. msg's destination is curLoc
        //NOTE: 'msg' may be coming from 'self' or from a remote PD. It can
        // either be a request (that may need a response) or a response.

        bool reqResponse = !!(msg->type & PD_MSG_REQ_RESPONSE); // for correctness check
        ocrLocation_t orgSrcLocation = msg->srcLocation; // for correctness check
        ocrPolicyDomainHcDist_t * pdSelfDist = (ocrPolicyDomainHcDist_t *) self;

        // NOTE: It is important to ensure the base processMessage call doesn't
        // store any pointers read from the request message
        ret = pdSelfDist->baseProcessMessage(self, msg, isBlocking);

        // Here, 'msg' content has potentially changed if a response was required
        // If msg's destination is not the current location anymore, it means we were
        // processing an incoming request from another PD. Send the response now.

        if (msg->destLocation != curLoc) {
            ASSERT(reqResponse); // double check not trying to answer when we shouldn't
            // For now a two-way is always between the same pair of src/dst.
            // Cannot answer to someone else on behalf of the original sender.
            ASSERT(msg->destLocation == orgSrcLocation);

            //IMPL: Because we are processing a two-way originating from another PD,
            // the message buffer is necessarily managed by the runtime (as opposed
            // to be on the user call stack calling in its PD).
            // Hence, we post the response as a one-way, persistent and no handle.
            // The message will be deallocated on one-way call completion.
            u32 sendProp = PERSIST_MSG_PROP;
            DPRINTF(DEBUG_LVL_VVERB,"Send response to %d after local processing of msg\n", msg->destLocation);
            ASSERT(msg->type & PD_MSG_RESPONSE);
            ASSERT((msg->type & PD_MSG_TYPE_ONLY) != PD_MSG_MGT_MONITOR_PROGRESS);
            switch(msg->type & PD_MSG_TYPE_ONLY) {
            case PD_MSG_DB_ACQUIRE:
            {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
                DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: post-process response, GUID=0x%lx serialize DB's ptr, dest is %d\n",
                        PD_MSG_FIELD(guid.guid), (int) msg->destLocation);
                sendProp |= ASYNC_MSG_PROP;
#undef PD_MSG
#undef PD_TYPE
            break;
            }
            case PD_MSG_WORK_CREATE:
            {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_WORK_CREATE
                DPRINTF(DEBUG_LVL_VVERB,"PD_MSG_WORK_CREATE: post-process response, GUID=0x%lx modify paramv to avoid marshalling\n",
                        PD_MSG_FIELD(guid.guid));
                PD_MSG_FIELD(paramv) = NULL;
#undef PD_MSG
#undef PD_TYPE
            break;
            }
            default: { }
            }
            // Send the response message
            self->fcts.sendMessage(self, msg->destLocation, msg, NULL, sendProp);
        } else {
            // local post-processing
            if ((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_MGT_MONITOR_PROGRESS) {
                // This is to temporarily help with the shutdown process see issue #200
                hal_lock32(&((ocrPolicyDomainHcDist_t*)self)->piledWorkerCtxLock);
                ((ocrPolicyDomainHcDist_t*)self)->piledWorkerCtx--;
                hal_unlock32(&((ocrPolicyDomainHcDist_t*)self)->piledWorkerCtxLock);
            }
        }
    }
    PROCESS_MESSAGE_RETURN_NOW(self, ret);
}

u8 hcDistPdSendMessage(ocrPolicyDomain_t* self, ocrLocation_t target, ocrPolicyMsg_t *message,
                   ocrMsgHandle_t **handle, u32 properties) {
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    ASSERT(((int)target) > -1);
    ASSERT(message->srcLocation == self->myLocation);
    ASSERT(message->destLocation != self->myLocation);

    //DIST-TODO sep-concern: The PD should not know about underlying worker impl
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    int id = hcWorker->id;
    u8 ret = self->commApis[id]->fcts.sendMessage(self->commApis[id], target, message, handle, properties);

    return ret;
}

u8 hcDistPdPollMessage(ocrPolicyDomain_t *self, ocrMsgHandle_t **handle) {
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    //DIST-TODO sep-concern: The PD should not know about underlying worker impl
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    int id = hcWorker->id;
    u8 ret = self->commApis[id]->fcts.pollMessage(self->commApis[id], handle);

    return ret;
}

u8 hcDistPdWaitMessage(ocrPolicyDomain_t *self,  ocrMsgHandle_t **handle) {
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    //DIST-TODO sep-concern: The PD should not know about underlying worker impl
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    int id = hcWorker->id;
    u8 ret = self->commApis[id]->fcts.waitMessage(self->commApis[id], handle);

    return ret;
}

void hcDistPolicyDomainStart(ocrPolicyDomain_t * pd) {
    ocrPolicyDomainHcDist_t * distPd = (ocrPolicyDomainHcDist_t *) pd;
    // initialize parent
    distPd->baseStart(pd);
}

void hcDistPolicyDomainStop(ocrPolicyDomain_t * pd) {
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    // In OCR-distributed we need to drain messages from the
    // communication sub-system. This is to make sure the
    // runtime doesn't abruptely stop while shutdown messages
    // are being issued.
    // DIST-TODO stop: standardize runlevels
    int runlevel = 3;
    u64 i = 0;
    while (runlevel > 0) {
        DPRINTF(DEBUG_LVL_VVERB,"HC-DIST stop begin at RL %d\n", runlevel);
        u64 maxCount = pd->workerCount;
        for(i = 0; i < maxCount; i++) {
            //DIST-TODO stop: until runlevels get standardized, we only
            //make communication workers go through runlevels
            ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) pd->workers[i];
            if (hcWorker->hcType == HC_WORKER_COMM) {
                DPRINTF(DEBUG_LVL_VVERB,"HC-DIST stop comm-worker at RL %d\n", runlevel);
                pd->workers[i]->fcts.stop(pd->workers[i]);
            }
        }
        //DIST-TODO stop: Same thing here we assume all the comm-api are runlevel compatible
        maxCount = pd->commApiCount;
        for(i = 0; i < maxCount; i++) {
            DPRINTF(DEBUG_LVL_VVERB,"HC-DIST stop comm-api at RL %d\n", runlevel);
            pd->commApis[i]->fcts.stop(pd->commApis[i]);
        }
        DPRINTF(DEBUG_LVL_VVERB,"HC-DIST stop end at RL %d END\n", runlevel);
        runlevel--;
    }

    // call the regular stop
    DPRINTF(DEBUG_LVL_VVERB,"HC-DIST stop calling base stop\n");
    ocrPolicyDomainHcDist_t * distPd = (ocrPolicyDomainHcDist_t *) pd;
    distPd->baseStop(pd);
}

ocrPolicyDomain_t * newPolicyDomainHcDist(ocrPolicyDomainFactory_t * factory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainHcDist_t * derived = (ocrPolicyDomainHcDist_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainHcDist_t), NULL);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

#ifdef OCR_ENABLE_STATISTICS
    factory->initialize(factory, statsObject, base, costFunction, perInstance);
#else
    factory->initialize(factory, base, costFunction, perInstance);
#endif
    return base;
}

void initializePolicyDomainHcDist(ocrPolicyDomainFactory_t * factory,
#ifdef OCR_ENABLE_STATISTICS
                                  ocrStats_t *statsObject,
#endif
                                  ocrPolicyDomain_t *self, ocrCost_t *costFunction, ocrParamList_t *perInstance) {
    ocrPolicyDomainFactoryHcDist_t * derivedFactory = (ocrPolicyDomainFactoryHcDist_t *) factory;
    // Initialize the base policy-domain
#ifdef OCR_ENABLE_STATISTICS
    derivedFactory->baseInitialize(factory, statsObject, self, costFunction, perInstance);
#else
    derivedFactory->baseInitialize(factory, self, costFunction, perInstance);
#endif
    ocrPolicyDomainHcDist_t * hcDistPd = (ocrPolicyDomainHcDist_t *) self;
    hcDistPd->baseProcessMessage = derivedFactory->baseProcessMessage;
    hcDistPd->baseStart = derivedFactory->baseStart;
    hcDistPd->baseStop = derivedFactory->baseStop;
    hcDistPd->shutdownSent = false;
    hcDistPd->shutdownAckCount = 0;
    hcDistPd->piledWorkerCtx = 0;
    hcDistPd->piledWorkerCtxLock = 0;
    hcDistPd->lockDbLookup = 0;
}

static void destructPolicyDomainFactoryHcDist(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcDist(ocrParamList_t *perType) {
    ocrPolicyDomainFactory_t * baseFactory = newPolicyDomainFactoryHc(perType);
    ocrPolicyDomainFcts_t baseFcts = baseFactory->policyDomainFcts;

    ocrPolicyDomainFactoryHcDist_t* derived = (ocrPolicyDomainFactoryHcDist_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryHcDist_t), (void *)10);
    ocrPolicyDomainFactory_t* derivedBase = (ocrPolicyDomainFactory_t*) derived;
    // Set up factory function pointers and members
    derivedBase->instantiate = newPolicyDomainHcDist;
    derivedBase->initialize = initializePolicyDomainHcDist;
    derivedBase->destruct =  destructPolicyDomainFactoryHcDist;
    derivedBase->policyDomainFcts = baseFcts;
    derived->baseInitialize = baseFactory->initialize;
    derived->baseStart = baseFcts.start;
    derived->baseStop = baseFcts.stop;
    derived->baseProcessMessage = baseFcts.processMessage;

    // specialize some of the function pointers
    derivedBase->policyDomainFcts.start = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcDistPolicyDomainStart);
    derivedBase->policyDomainFcts.stop = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), hcDistPolicyDomainStop);
    derivedBase->policyDomainFcts.processMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*,ocrPolicyMsg_t*,u8), hcDistProcessMessage);
    derivedBase->policyDomainFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrLocation_t, ocrPolicyMsg_t *, ocrMsgHandle_t**, u32),
                                                   hcDistPdSendMessage);
    derivedBase->policyDomainFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), hcDistPdPollMessage);
    derivedBase->policyDomainFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), hcDistPdWaitMessage);

    baseFactory->destruct(baseFactory);
    return derivedBase;
}

#endif /* ENABLE_POLICY_DOMAIN_HC_DIST */
