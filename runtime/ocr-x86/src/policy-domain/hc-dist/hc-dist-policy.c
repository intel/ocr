
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

// Data-structure used to store foreign DB information
//DIST-TODO cloning: This is a poor's man meta-data cloning for datablocks
typedef struct {
    u64 size;
    void * volatile ptr;
    u32 flags;
    u32 count;
} ProxyDb_t;

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
            PD_MSG_FIELD(size) = 0ULL;
            u8 returnCode = self->fcts.processMessage(self, &msgClone, true);
            ASSERT(returnCode == 0);
            // On return, Need some more post-processing to make a copy of the metadata
            // and set the fatGuid's metadata ptr to point to the copy
            void * metaDataPtr = self->fcts.pdMalloc(self, metaDataSize);
            ASSERT(PD_MSG_FIELD(guid.metaDataPtr) != NULL);
            ASSERT(PD_MSG_FIELD(guid.guid) == remoteGuid);
            ASSERT(PD_MSG_FIELD(size) == metaDataSize);
            hal_memCopy(metaDataPtr, PD_MSG_FIELD(guid.metaDataPtr), metaDataSize, false);
            //DIST-TODO Potentially multiple concurrent registerGuid on the same template
            // TEMP DEBUG
            ocrGuidKind tkind;
            self->guidProviders[0]->fcts.getKind(self->guidProviders[0], remoteGuid, &tkind);
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

static void releaseLocalDb(ocrPolicyDomain_t * pd, ocrGuid_t dbGuid, u64 size) {
    ocrTask_t *curTask = NULL;
    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, &curTask, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = dbGuid;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(edt.guid) = curTask->guid;
    PD_MSG_FIELD(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD(size) = size;
    PD_MSG_FIELD(properties) = DB_PROP_RT_ACQUIRE; // Runtime release
    PD_MSG_FIELD(returnDetail) = 0;
    // Ignore failures at this point
    pd->fcts.processMessage(pd, &msg, true);
#undef PD_MSG
#undef PD_TYPE
}

static void * findMsgPayload(ocrPolicyMsg_t * msg, u32 * size) {
    u64 msgHeaderSize = sizeof(ocrPolicyMsg_t);
    if (size != NULL) {
        *size = (msg->size - msgHeaderSize);
    }
    return ((char*) msg) + msgHeaderSize;
}

static void processDbAcquireResponse(ocrPolicyDomain_t * self, ocrPolicyMsg_t * response) {
#define PD_MSG (response)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    if (PD_MSG_FIELD(properties) & DB_FLAG_RT_FETCH) {
        DPRINTF(DEBUG_LVL_VVERB,"Received DB_ACQUIRE FETCH reply ptr=%p size=%lu properties=0x%x\n",
                response, PD_MSG_FIELD(size), PD_MSG_FIELD(properties));
        ocrPolicyDomainHcDist_t * typedSelf = (ocrPolicyDomainHcDist_t *) self;
        // double check if someone went ahead of us
        hal_lock32(&(typedSelf->proxyLock));
        u64 val;
        self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
        ASSERT(val != 0); // because we always pre-create the dbProxy
        ProxyDb_t * dbProxy = (ProxyDb_t*) val;
        if (dbProxy->ptr == NULL) {
            ASSERT(dbProxy->count >= 1);
            // The acquire doubles as a fetch, need to deserialize DB's data from the message
            PD_MSG_FIELD(properties) &= ~DB_FLAG_RT_FETCH;
            // Set the pointer to the message payload
            void * ptr = findMsgPayload(response, NULL);
            u64 size = PD_MSG_FIELD(size);
            // Copy data ptr from message to new buffer
            void * newPtr = self->fcts.pdMalloc(self, size);
            hal_memCopy(newPtr, ptr, size, false);
            PD_MSG_FIELD(ptr) = newPtr;
            // If the acquired DB is not RO or NCR or single assignment
            // we'll need to write it back on release
            bool doWriteBack = !((PD_MSG_FIELD(properties) & DB_MODE_NCR) || (PD_MSG_FIELD(properties) & DB_MODE_RO) ||
                                 (PD_MSG_FIELD(properties) & DB_PROP_SINGLE_ASSIGNMENT));
            if (doWriteBack) {
                PD_MSG_FIELD(properties) |= DB_FLAG_RT_WRITE_BACK;
            }
            // Set up the dbProxy
            // ProxyDb_t * dbProxy = self->fcts.pdMalloc(self, sizeof(ProxyDb_t));
            dbProxy->size = PD_MSG_FIELD(size);
            dbProxy->flags = PD_MSG_FIELD(properties);
            dbProxy->ptr = PD_MSG_FIELD(ptr);
            // self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], PD_MSG_FIELD(guid.guid), (u64) dbProxy);
            DPRINTF(DEBUG_LVL_VVERB,"Commit DB_ACQUIRE FETCH db proxy setup guid=0x%lx ptr=%p size=%lu properties=0x%x\n",
                (int) self->myLocation, PD_MSG_FIELD(guid.guid), dbProxy->ptr, dbProxy->size, dbProxy->flags);
        } else {
            ProxyDb_t * dbProxy = (ProxyDb_t*) val;
            ASSERT(dbProxy->ptr != NULL);
            DPRINTF(DEBUG_LVL_VVERB,"Discard Commit DB_ACQUIRE FETCH reply ptr=%p size=%lu properties=0x%x\n",
                    dbProxy->ptr, dbProxy->size, dbProxy->flags);
            PD_MSG_FIELD(size) = dbProxy->size;
            PD_MSG_FIELD(ptr) = dbProxy->ptr;
        }
        hal_unlock32(&(typedSelf->proxyLock));
    } else {
        DPRINTF(DEBUG_LVL_VVERB,"Received DB_ACQUIRE reply\n");
        // Acquire without fetch (Runtime detected data was already available locally and usable).
        // Hence the message size should be the regular policy message size
        ASSERT(response->size == sizeof(ocrPolicyMsg_t));
        // Here we're potentially racing with an acquire populating the db proxy
        u64 val;
        self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
        ASSERT(val != 0); // The registration should ensure there's a DB proxy set up
        ProxyDb_t * dbProxy = (ProxyDb_t*) val;
        //DIST-TODO db: this is borderline. Since the other worker is populating several fields,
        //when 'ptr' is written it is not guaranteed the other assignment are visible yet.
        DPRINTF(DEBUG_LVL_VVERB,"Waiting for DB_ACQUIRE reply guid=0x%lx ptr=%p size=%lu properties=0x%x \n",
                PD_MSG_FIELD(guid.guid), dbProxy->ptr, dbProxy->size, dbProxy->flags);
        ASSERT(dbProxy->ptr != NULL);
        // while(dbProxy->ptr == NULL); // busy-wait for data to become available
        DPRINTF(DEBUG_LVL_VVERB,"Received DB_ACQUIRE reply ptr=%p size=%lu properties=0x%x\n",
                dbProxy->ptr, dbProxy->size, dbProxy->flags);
        PD_MSG_FIELD(size) = dbProxy->size;
        PD_MSG_FIELD(ptr) = dbProxy->ptr;
        // Warning: Need to update the proxy flags as it could be that previously the DB
        // was owned in RO and we're now acquiring in a mode that requires a write-back.
        bool doWriteBack = !((PD_MSG_FIELD(properties) & DB_MODE_NCR) || (PD_MSG_FIELD(properties) & DB_MODE_RO) ||
                             (PD_MSG_FIELD(properties) & DB_PROP_SINGLE_ASSIGNMENT));
        if (doWriteBack) {
            DPRINTF(DEBUG_LVL_VVERB,"Update dbProxy WB FLAG for %lx\n", PD_MSG_FIELD(guid.guid));
            dbProxy->flags |= DB_FLAG_RT_WRITE_BACK;
        }
    }
#undef PD_MSG
#undef PD_TYPE
}
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

    //DIST-TODO msg setup: message's size should be set systematically by caller. Do it here as a failsafe for now
    msg->size = sizeof(ocrPolicyMsg_t);

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
            return OCR_EINVAL;
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
                PD_MSG_FIELD(properties) = false; // not called from add-dependence
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
            hcDistReleasePd((ocrPolicyDomainHc_t*)self);
            return 0;
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
            hcDistReleasePd((ocrPolicyDomainHc_t*)self);
            return 0;
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
                // Incoming acquire: Need to resolve the DB metadata before handing the message over
                // to the base policy-domain. (the sender didn't know about the metadataPtr, receiver does)
                u64 val;
                self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
                ASSERT(val != 0);
                PD_MSG_FIELD(guid.metaDataPtr) = (void *) val;
                DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: received request for DB GUID 0x%lx with properties=0x%x\n",
                        PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
            }
            if ((msg->srcLocation == curLoc) && (msg->destLocation != curLoc)) {
                // Outgoing acquire
                DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: remote request for DB GUID 0x%lx with properties=0x%x\n",
                        PD_MSG_FIELD(guid.guid), PD_MSG_FIELD(properties));
                // First check if we know about this foreign DB
                ocrPolicyDomainHcDist_t * typedSelf = (ocrPolicyDomainHcDist_t *) self;
                hal_lock32(&(typedSelf->proxyLock));
                u64 val;
                self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
                ProxyDb_t * proxyDb = NULL;
                if (val == 0) {
                    // Don't know about this DB yet, request a fetch
                    //There's a race here between first timers DB_ACQUIRE competing to fetch the DB
                    //We would like to have a single worker doing the fetch to avoid wasting bandwidth
                    //However coordinating the two can lead to deadlock in current implementation + blocking helper mode.
                    //So, for now allow concurrent fetch and a single one will be allowed to commit to the registerGuid
                    //DIST-TODO db: Revisit this locking scheme when we have a better idea of what we want to do with DB management
                    DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: remote request: DB guid 0x%lx msg=%p requires FETCH\n", PD_MSG_FIELD(guid.guid), msg);
                    PD_MSG_FIELD(properties) |= DB_FLAG_RT_FETCH;
                    proxyDb = self->fcts.pdMalloc(self, sizeof(ProxyDb_t));
                    proxyDb->size = 0;
                    proxyDb->ptr = NULL;
                    proxyDb->flags = 0;
                    proxyDb->count = 0;
                    self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], PD_MSG_FIELD(guid.guid), (u64) proxyDb);
                } else {
                    proxyDb = (ProxyDb_t *) val;
                }

                if (proxyDb->ptr == NULL) {
                    // If data is not there, then try to do a concurrent FETCH
                    // This is important just in case this acquire response arrives
                    // before the other one (that created the proxyDb) or others in here.
                    // see comment above for more info
                    PD_MSG_FIELD(properties) |= DB_FLAG_RT_FETCH;
                }
                //register ourselves
                proxyDb->count++;
                hal_unlock32(&(typedSelf->proxyLock));
            }
        } else {
            ASSERT(msg->type & PD_MSG_RESPONSE);
            RETRIEVE_LOCATION_FROM_MSG(self, edt, msg->destLocation)
            if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
                // Incoming remote DB_ACQUIRE response, pre-process response
                processDbAcquireResponse(self, msg);
            } // else outgoing response to be sent out, fall-through
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
        if ((msg->srcLocation != curLoc) && (msg->destLocation == curLoc)) {
            // Incoming DB_RELEASE pre-processing
            // Need to resolve the DB metadata locally before handing the message over
            u64 val;
            self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
            ASSERT(val != 0);
            PD_MSG_FIELD(guid.metaDataPtr) = (void *) val;
            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE received request for DB GUID 0x%lx WB=%d\n",
                    PD_MSG_FIELD(guid.guid), !!(PD_MSG_FIELD(properties) & DB_FLAG_RT_WRITE_BACK));
            //DIST-TODO db: We may want to double check this writeback (first one) is legal wrt single assignment
            if (PD_MSG_FIELD(properties) & DB_FLAG_RT_WRITE_BACK) {
                // Unmarshall and write back
                DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE received request: unmarshall DB guid 0x%lx ptr for local WB\n", PD_MSG_FIELD(guid.guid));
                //Important to read from the RELEASE size u64 field instead of the msg size (u32)
                u64 size = PD_MSG_FIELD(size);
                void * data = findMsgPayload(msg, NULL);
                // Acquire local DB on behalf of remote release to do the writeback.
                // Do it in OB mode so as to make sure the acquire goes through
                //TODO DBX DB_MODE_NCR is not implemented, we're converting the call to a special
                void * localData = acquireLocalDb(self, PD_MSG_FIELD(guid.guid), DB_MODE_NCR);
                ASSERT(localData != NULL);
                hal_memCopy(localData, data, size, false);
                // TODO DBX We do not release here because we've been using this special mode to do the write back
                // releaseLocalDb(self, PD_MSG_FIELD(guid.guid), size);
            } // else fall-through and do the regular release
        }
        if ((msg->srcLocation == curLoc) && (msg->destLocation != curLoc)) {
            // Outgoing DB_RELEASE pre-processing
            ocrPolicyDomainHcDist_t * typedSelf = (ocrPolicyDomainHcDist_t *) self;
            hal_lock32(&(typedSelf->proxyLock));
            u64 val;
            self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
            ASSERT(val != 0);
            ProxyDb_t * proxyDb = (ProxyDb_t *) val;
            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE sending request for DB GUID 0x%lx with properties=0x%x and count=%d\n",
                    PD_MSG_FIELD(guid.guid), proxyDb->flags,proxyDb->count);
            ASSERT(proxyDb->count > 0);
            // Only do the write back when the last EDT is checking out
            if (proxyDb->count == 1) {
                // Need to make sure the write back is done before we decrement the counter
                // to zero
                if (proxyDb->flags & DB_FLAG_RT_WRITE_BACK) {
                    PD_MSG_FIELD(properties) = DB_FLAG_RT_WRITE_BACK;
                    if (proxyDb->flags & DB_PROP_SINGLE_ASSIGNMENT) {
                        // Clear the writeback flag. For single-assignment DB this prevent multiple illegal write-backs
                        proxyDb->flags &= ~DB_FLAG_RT_WRITE_BACK;
                    }
                    // Shipping the DB's data back to the home node
                    DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE sending request: marshall DB GUID 0x%lx for remote WB\n", PD_MSG_FIELD(guid.guid));
                    u64 dbSize = proxyDb->size;
                    void * dbPtr = proxyDb->ptr;
                    // Prepare response message
                    u64 msgHeaderSize = sizeof(ocrPolicyMsg_t);
                    u64 msgSize = msgHeaderSize + dbSize;
                    ocrPolicyMsg_t * newMessage = self->fcts.pdMalloc(self, msgSize); //DIST-TODO: should be able to use RELEASE real size + dbSize
                    getCurrentEnv(NULL, NULL, NULL, newMessage);
                    // Copy header
                    hal_memCopy(newMessage, msg, msgHeaderSize, false);
                    newMessage->size = msgSize;
                    void * dbCopyPtr = ((char*)newMessage) + msgHeaderSize;
                    //Copy DB's data in the message's payload
                    hal_memCopy(dbCopyPtr, dbPtr, dbSize, false);
                    msg = newMessage;
                    PD_MSG_FIELD(size) = dbSize;
                } else {
                    // destroy the DB proxy
                    proxyDb->count--;
                    self->fcts.pdFree(self, proxyDb->ptr);
                    self->fcts.pdFree(self, proxyDb);
                    // cleanup guid provider entry to detect racy acquire
                    self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], PD_MSG_FIELD(guid.guid), (u64) 0);
                }
            } else {
                // else: DB is still in use locally, send release without taking into account the eventual WB flag
                proxyDb->count--;
            }
            hal_unlock32(&(typedSelf->proxyLock));
            // All fall-through to send
        }
        if ((msg->srcLocation == curLoc) && (msg->destLocation == curLoc)) {
            DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE local processing: DB GUID 0x%lx\n", PD_MSG_FIELD(guid.guid));
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
                        hcDistReleasePd((ocrPolicyDomainHc_t*)self);
                        return 0;
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
        // of another PD and let him know after the fact. In that case, the PD may
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
                // Receiving the reply for a DB_ACQUIRE
                processDbAcquireResponse(self, response);
            break;
            }
            case PD_MSG_DB_CREATE:
            {
                // Receiving the reply for a DB_CREATE
                // Create a db proxy
#define PD_MSG (response)
#define PD_TYPE PD_MSG_DB_CREATE
                ocrGuid_t dbGuid = PD_MSG_FIELD(guid.guid);
                u64 size = PD_MSG_FIELD(size);
                // Create the local ptr
                void * ptr = self->fcts.pdMalloc(self, size);
                // Double check there's no previous registration
                u64 val;
                self->guidProviders[0]->fcts.getVal(self->guidProviders[0], dbGuid, &val, NULL);
                ASSERT(val == 0);
                // Register the proxy DB
                ProxyDb_t * proxyDb = self->fcts.pdMalloc(self, sizeof(ProxyDb_t));
                proxyDb->size = size;
                proxyDb->ptr = ptr;
                // Preset the writeback flag: even single assignment needs to be written back the first time.
                proxyDb->flags = PD_MSG_FIELD(properties) | DB_FLAG_RT_WRITE_BACK;
                proxyDb->count = 1; // self
                self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], dbGuid, (u64) proxyDb);
                PD_MSG_FIELD(ptr) = ptr;
#undef PD_MSG
#undef PD_TYPE
            break;
            }
            case PD_MSG_DB_RELEASE:
            {
#define PD_MSG (response)
#define PD_TYPE PD_MSG_DB_RELEASE
                // We need to check if the write-back that occurred is the last use of the DB
                if (PD_MSG_FIELD(properties) & DB_FLAG_RT_WRITE_BACK) {
                    ocrPolicyDomainHcDist_t * typedSelf = (ocrPolicyDomainHcDist_t *) self;
                    hal_lock32(&(typedSelf->proxyLock));
                    // We did not decrement the proxy counter when we posted the WB,
                    // so we're still a user of the DB and it cannot have been removed
                    // from the guidProvider
                    u64 val;
                    self->guidProviders[0]->fcts.getVal(self->guidProviders[0], PD_MSG_FIELD(guid.guid), &val, NULL);
                    ASSERT(val != 0);
                    ProxyDb_t * proxyDb = (ProxyDb_t *) val;
                    ASSERT(proxyDb->count > 0);
                    proxyDb->count--;
                    DPRINTF(DEBUG_LVL_VVERB,"DB_RELEASE WB post-process on DB GUID 0x%lx with properties=0x%x and count=%d\n",
                            PD_MSG_FIELD(guid.guid), proxyDb->flags, proxyDb->count);
                    if (proxyDb->count == 0) { // Did the WB and last user
                        self->fcts.pdFree(self, proxyDb->ptr);
                        self->fcts.pdFree(self, proxyDb);
                        self->guidProviders[0]->fcts.registerGuid(self->guidProviders[0], PD_MSG_FIELD(guid.guid), (u64) 0);
                    } // else someone else is using this cached version
                    hal_unlock32(&(typedSelf->proxyLock));
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

            //Since the caller only has access to the original message we need
            //to make sure it's up-to-date

            if (originalMsg != msg) {
                // There's been a request message copy (maybe to accomodate larger outgoing message payload)
                u64 fullMsgSize = 0, marshalledSize = 0;
                ocrPolicyMsgGetMsgSize(handle->response, &fullMsgSize, &marshalledSize);
                // For now
                ASSERT(fullMsgSize <= sizeof(ocrPolicyMsg_t));
                ocrPolicyMsgMarshallMsg(handle->response, (u8*)originalMsg, MARSHALL_DUPLICATE);

                // Check if the request message has also been used for the response
                if (msg != handle->response) {
                    self->fcts.pdFree(self, msg);
                }
                self->fcts.pdFree(self, handle->response);
            } else {
                if (msg != handle->response) {
                    // Response is in a different message, copy and free
                    u64 fullMsgSize = 0, marshalledSize = 0;
                    ocrPolicyMsgGetMsgSize(handle->response, &fullMsgSize, &marshalledSize);
                    // For now
                    ASSERT(fullMsgSize <= sizeof(ocrPolicyMsg_t));
                    ocrPolicyMsgMarshallMsg(handle->response, (u8*)originalMsg, MARSHALL_DUPLICATE);
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
                // Outgoing asynchronous response of a two-way communication
                // Should be the only asynchronous two-way msg kind for now
                ASSERT((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_DB_ACQUIRE);
                // Marshall outgoing DB_ACQUIRE Response
                switch(msg->type & PD_MSG_TYPE_ONLY) {
                case PD_MSG_DB_ACQUIRE:
                {
#define PD_MSG (msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
                    DPRINTF(DEBUG_LVL_VVERB,"DB_ACQUIRE: post-process response, guid=0x%lx serialize DB's ptr, dest is [%d]\n",
                            PD_MSG_FIELD(guid.guid), (int) msg->destLocation);
                    // The base policy acquired the DB on behalf of the remote EDT
                    if (PD_MSG_FIELD(properties) & DB_FLAG_RT_FETCH) {
                        // fetch was requested, serialize the DB data into response
                        u64 dbSize = PD_MSG_FIELD(size);
                        void * dbPtr = PD_MSG_FIELD(ptr);
                        // Prepare response message
                        u64 msgHeaderSize = sizeof(ocrPolicyMsg_t);
                        u64 msgSize = msgHeaderSize + dbSize;
                        ocrPolicyMsg_t * responseMessage = self->fcts.pdMalloc(self, msgSize); //DIST-TODO: should be able to use ACQUIRE real size + dbSize
                        getCurrentEnv(NULL, NULL, NULL, responseMessage);
                        hal_memCopy(responseMessage, msg, msgHeaderSize, false);
                        responseMessage->size = msgSize;
                        void * dbCopyPtr = findMsgPayload(responseMessage, NULL);
                        //Copy DB's data in the message's payload
                        hal_memCopy(dbCopyPtr, dbPtr, dbSize, false);
                        msg = responseMessage;
                    }
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
        ocrPolicyDomainHcDist_t * pdSelfDist = (ocrPolicyDomainHcDist_t *) self;
        ret = pdSelfDist->baseProcessMessage(self, msg, isBlocking);

        // Here, 'msg' content has potentially changed if a response was required
        // If msg's destination is not the current location anymore, it means we were
        // processing an incoming request from another PD. Send the response now.
        if (msg->destLocation != curLoc) {
            //IMPL: It also means the message buffer is managed by the runtime
            // (as opposed to be on the user call stack calling in its PD).
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
                // The base policy acquired the DB on behalf of the remote EDT
                if (PD_MSG_FIELD(properties) & DB_FLAG_RT_FETCH) {
                    // fetch was requested, serialize the DB data into response
                    u64 dbSize = PD_MSG_FIELD(size);
                    void * dbPtr = PD_MSG_FIELD(ptr);
                    // Prepare response message
                    u64 msgHeaderSize = sizeof(ocrPolicyMsg_t);
                    u64 msgSize = msgHeaderSize + dbSize;
                    ocrPolicyMsg_t * responseMessage = self->fcts.pdMalloc(self, msgSize); //DIST-TODO: should be able to use ACQUIRE real size + dbSize
                    getCurrentEnv(NULL, NULL, NULL, responseMessage);
                    hal_memCopy(responseMessage, msg, msgHeaderSize, false);
                    responseMessage->size = msgSize;
                    void * dbCopyPtr = findMsgPayload(responseMessage, NULL);
                    //Copy DB's data in the message's payload
                    hal_memCopy(dbCopyPtr, dbPtr, dbSize, false);
                    //NOTE: The DB will be released on DB_RELEASE
                    // Since this is a response to a remote PD request, the request
                    // message buffer is runtime managed so we need to deallocate it.
                    self->fcts.pdFree(self, msg);
                    msg = responseMessage;
                }
                sendProp |= ASYNC_MSG_PROP;
#undef PD_MSG
#undef PD_TYPE
            break;
            }
            default: { }
            }
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
    hcDistReleasePd((ocrPolicyDomainHc_t*)self);
    return ret;
}

u8 hcDistPdSendMessage(ocrPolicyDomain_t* self, ocrLocation_t target, ocrPolicyMsg_t *message,
                   ocrMsgHandle_t **handle, u32 properties) {
    ocrWorker_t * worker;
    getCurrentEnv(NULL, &worker, NULL, NULL);
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
    hcDistPd->proxyLock = 0;
    hcDistPd->shutdownSent = false;
    hcDistPd->shutdownAckCount = 0;
    hcDistPd->piledWorkerCtx = 0;
    hcDistPd->piledWorkerCtxLock = 0;
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
