/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_EVENT_HC

#include "ocr-hal.h"
#include "debug.h"
#include "event/hc/hc-event.h"
#include "ocr-datablock.h"
#include "ocr-edt.h"
#include "ocr-event.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-task.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"
#include "ocr-errors.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#endif

#define SEALED_LIST ((void *) -1)
#define END_OF_LIST NULL
#define UNINITIALIZED_DATA ((ocrGuid_t) -2)

// Change here if you want a different initial number
// of waiters and signalers
#define INIT_WAITER_COUNT 4
#define INIT_SIGNALER_COUNT 0

#define DEBUG_TYPE EVENT

/******************************************************/
/* OCR-HC Debug                                       */
/******************************************************/

#ifdef OCR_DEBUG
static char * eventTypeToString(ocrEvent_t * base) {
    ocrEventTypes_t type = base->kind;
    if(type == OCR_EVENT_ONCE_T) {
        return "once";
    } else if (type == OCR_EVENT_IDEM_T) {
        return "idem";
    } else if (type == OCR_EVENT_STICKY_T) {
        return "sticky";
    } else if (type == OCR_EVENT_LATCH_T) {
        return "latch";
    } else {
        return "unknown";
    }
}
#endif


/******************************************************/
/* OCR-HC Events Implementation                       */
/******************************************************/

//
// OCR-HC Single Events Implementation
//

u8 destructEventHc(ocrEvent_t *base) {

    ocrEventHc_t *event = (ocrEventHc_t*)base;

    ocrPolicyDomain_t *pd = NULL;
    PD_MSG_STACK(msg);
    ocrTask_t *curTask = NULL;
    getCurrentEnv(&pd, NULL, &curTask, &msg);

    DPRINTF(DEBUG_LVL_INFO, "Destroy %s: 0x%lx\n", eventTypeToString(base), base->guid);

#ifdef OCR_ENABLE_STATISTICS
    statsEVT_DESTROY(pd, getCurrentEDT(), NULL, base->guid, base);
#endif

    // Destroy both of the datablocks linked
    // with this event

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_FREE
    msg.type = PD_MSG_DB_FREE | PD_MSG_REQUEST;
    PD_MSG_FIELD_I(guid) = event->waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_I(properties) = 0;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));

    /* Signalers not used
    msg.type = PD_MSG_DB_FREE | PD_MSG_REQUEST;
    PD_MSG_FIELD_I(guid) = event->signalersDb;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
    */
#undef PD_MSG
#undef PD_TYPE
    // Now destroy the GUID
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    // These next two statements may be not required. Just to be safe
    PD_MSG_FIELD_I(guid.guid) = base->guid;
    PD_MSG_FIELD_I(guid.metaDataPtr) = base;
    PD_MSG_FIELD_I(properties) = 1; // Free metadata
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

ocrFatGuid_t getEventHc(ocrEvent_t *base) {
    ocrFatGuid_t res = {.guid = NULL_GUID, .metaDataPtr = NULL};
    switch(base->kind) {
    case OCR_EVENT_ONCE_T:
    case OCR_EVENT_LATCH_T:
        break;
    case OCR_EVENT_STICKY_T:
    case OCR_EVENT_IDEM_T: {
        ocrEventHcPersist_t *event = (ocrEventHcPersist_t*)base;
        res.guid = (event->data == UNINITIALIZED_GUID)?ERROR_GUID:event->data;
        break;
    }
    default:
        ASSERT(0);
    }
    return res;
}

// For once events, we don't have to worry about
// concurrent registerWaiter calls (this would be a programmer error)
u8 satisfyEventHcOnce(ocrEvent_t *base, ocrFatGuid_t db, u32 slot) {
    ocrEventHc_t *event = (ocrEventHc_t*)base;
    ASSERT(slot == 0); // For non-latch events, only one slot

    DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx with 0x%lx\n", eventTypeToString(base),
            base->guid, db.guid);

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    getCurrentEnv(&pd, NULL, &curTask, &msg);
    ocrFatGuid_t currentEdt;
    currentEdt.guid = (curTask == NULL) ? NULL_GUID : curTask->guid;
    currentEdt.metaDataPtr = curTask;

#ifdef OCR_ENABLE_STATISTICS
    statsDEP_SATISFYToEvt(pd, currentEdt.guid, NULL, base->guid, base, data, slotEvent);
#endif

    u32 i;
    regNode_t *waiters = NULL;
    if(event->waitersCount) {
        // First acquire the DB that contains the waiters
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
        msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->waitersDb;
        PD_MSG_FIELD_IO(edt) = currentEdt;
        PD_MSG_FIELD_IO(properties) = DB_MODE_RO | DB_PROP_RT_ACQUIRE;
        //Should be a local DB
        u8 res = pd->fcts.processMessage(pd, &msg, true);
        ASSERT(!res);
        waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
        // Todo: Related to bug #273. We should not get an updated deguidification...
        event->waitersDb = PD_MSG_FIELD_IO(guid); //Get updated deguidifcation if needed
        ASSERT(waiters);
#undef PD_TYPE

        // Second, call satisfy on all the waiters
#define PD_TYPE PD_MSG_DEP_SATISFY
        // Fill the common part to all the satisfy messages
        PD_MSG_FIELD_I(payload) = db; // Do this once as it may get resolved the first time
        PD_MSG_FIELD_I(currentEdt) = currentEdt;
        PD_MSG_FIELD_I(properties) = 0;

        for(i = 0; i < event->waitersCount; ++i) {
#ifdef OCR_ENABLE_STATISTICS
            // We do this *before* calling signalWaiter because otherwise
            // if the event is a OCR_EVENT_ONCE_T, it will get freed
            // before we can get the message sent
            statsDEP_SATISFYFromEvt(pd, base->guid, base, waiter->guid,
                                    data.guid, waiter->slot);
#endif
            DPRINTF(DEBUG_LVL_INFO, "SatisfyFromEvent %s: 0x%lx with 0x%lx\n", eventTypeToString(base),
            waiters[i].guid, base->guid);

            getCurrentEnv(NULL, NULL, NULL, &msg);
            msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
            PD_MSG_FIELD_I(guid.guid) = waiters[i].guid;
            PD_MSG_FIELD_I(guid.metaDataPtr) = NULL;
            PD_MSG_FIELD_I(slot) = waiters[i].slot;
            RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
        }

        // Release the DB
#undef PD_TYPE
#define PD_TYPE PD_MSG_DB_RELEASE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->waitersDb;
        PD_MSG_FIELD_I(edt) = currentEdt;
        PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE
    }
    // Since this a ONCE event, we need to destroy it as well
    // This is safe to do so at this point as all the messages have been sent
    return destructEventHc(base);
}

// This is for persistent events such as sticky or idempotent.
u8 satisfyEventHcPersist(ocrEvent_t *base, ocrFatGuid_t db, u32 slot) {
    ocrEventHcPersist_t *event = (ocrEventHcPersist_t*)base;
    ASSERT(slot == 0); // Persistent-events are single slot
    ASSERT((event->base.base.kind == OCR_EVENT_IDEM_T) ||
           (event->data == UNINITIALIZED_GUID)); // Sticky events can be satisfied once

    if(event->data != UNINITIALIZED_GUID) {
        return 1; // TODO: Put some error code here.
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx with 0x%lx\n", eventTypeToString(base),
                base->guid, db.guid);

#ifdef OCR_ENABLE_STATISTICS
        ocrPolicyDomain_t *pd = getCurrentPD();
        ocrGuid_t edt = getCurrentEDT();
        statsDEP_SATISFYToEvt(pd, edt, NULL, base->guid, base, data, slotEvent);
#endif
    }

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    u32 i;
    regNode_t *waiters = NULL;
    u32 waitersCount = 0;
    getCurrentEnv(&pd, NULL, &curTask, &msg);

    // Try to get all the waiters
    hal_lock32(&(event->waitersLock));
    event->data = db.guid;
    waitersCount = event->base.waitersCount;
    event->base.waitersCount = (u32)-1; // Indicate that the event is satisfied
    hal_unlock32(&(event->waitersLock));

    // Acquire the DB that contains the waiters
    if(waitersCount) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
        msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
        PD_MSG_FIELD_IO(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_IO(edt.metaDataPtr) = curTask;
        // !! Warning !! ITW here (and not RO) works in pair with the lock
        // being unlocked before DB_RELEASE is called in 'registerWaiterEventHcPersist'
        PD_MSG_FIELD_IO(properties) = DB_MODE_ITW | DB_PROP_RT_ACQUIRE;
        //Should be a local DB
        u8 res = pd->fcts.processMessage(pd, &msg, true);
        ASSERT(!res);
        waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
        // TODO: bug #273
        event->base.waitersDb = PD_MSG_FIELD_IO(guid);
        ASSERT(waiters);
#undef PD_TYPE
        // Call satisfy on all the waiters
#define PD_TYPE PD_MSG_DEP_SATISFY
        PD_MSG_FIELD_I(payload) = db; // Do this once as it may get resolved the first time
        PD_MSG_FIELD_I(properties) = 0;
        PD_MSG_FIELD_I(satisfierGuid.guid) = base->guid;
        PD_MSG_FIELD_I(satisfierGuid.metaDataPtr) = base;
        for(i = 0; i < waitersCount; ++i) {
#ifdef OCR_ENABLE_STATISTICS
            // We do this *before* calling signalWaiter because otherwise
            // if the event is a OCR_EVENT_ONCE_T, it will get freed
            // before we can get the message sent
            statsDEP_SATISFYFromEvt(pd, base->guid, base, waiter->guid,
                                    data.guid, waiter->slot);
#endif
            DPRINTF(DEBUG_LVL_INFO, "SatisfyFromEvent %s: 0x%lx with 0x%lx\n", eventTypeToString(base),
            waiters[i].guid, base->guid);

            getCurrentEnv(NULL, NULL, NULL, &msg);
            msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
            PD_MSG_FIELD_I(guid.guid) = waiters[i].guid;
            PD_MSG_FIELD_I(guid.metaDataPtr) = NULL;
            PD_MSG_FIELD_I(slot) = waiters[i].slot;
            RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
        }
        // Release the DB
#undef PD_TYPE
#define PD_TYPE PD_MSG_DB_RELEASE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
        PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
        PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE
        //TODO early release of the DB here ?
    }
    // The event sticks around until the user destroys it
    return 0;
}

// This is for latch events
u8 satisfyEventHcLatch(ocrEvent_t *base, ocrFatGuid_t db, u32 slot) {
    ocrEventHcLatch_t *event = (ocrEventHcLatch_t*)base;
    ASSERT(slot == OCR_EVENT_LATCH_DECR_SLOT ||
           slot == OCR_EVENT_LATCH_INCR_SLOT);

    s32 incr = (slot == OCR_EVENT_LATCH_DECR_SLOT)?-1:1;
    s32 count;
    do {
        count = event->counter;
    } while(hal_cmpswap32(&(event->counter), count, count+incr) != count);

    DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx %s\n", eventTypeToString(base),
            base->guid, ((slot == OCR_EVENT_LATCH_DECR_SLOT) ? "decr":"incr"));

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    getCurrentEnv(&pd, NULL, &curTask, &msg);
    ocrFatGuid_t currentEdt;
    currentEdt.guid = (curTask == NULL) ? NULL_GUID : curTask->guid;
    currentEdt.metaDataPtr = curTask;

#ifdef OCR_ENABLE_STATISTICS
    statsDEP_SATISFYToEvt(pd, currentEdt.guid, NULL, base->guid, base, data, slot);
#endif
    if(count + incr != 0) {
        return 0;
    }
    // Here the event is satisfied
    DPRINTF(DEBUG_LVL_INFO, "Satisfy %s: 0x%lx reached zero\n", eventTypeToString(base), base->guid);
    u32 i;
    regNode_t *waiters = NULL;

    // Acquire the DB that contains the waiters
    if(event->base.waitersCount) {
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
        msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
        PD_MSG_FIELD_IO(edt) = currentEdt;
        PD_MSG_FIELD_IO(properties) = DB_MODE_RO | DB_PROP_RT_ACQUIRE;
        //Should be a local DB
        u8 res = pd->fcts.processMessage(pd, &msg, true);
        ASSERT(!res);

        waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
        // TODO: See bug #273
        event->base.waitersDb = PD_MSG_FIELD_IO(guid);
        ASSERT(waiters);
#undef PD_TYPE
        // Call satisfy on all the waiters
#define PD_TYPE PD_MSG_DEP_SATISFY
        PD_MSG_FIELD_I(payload) = db; // Do this once as it may get resolved the first time
        PD_MSG_FIELD_I(currentEdt) = currentEdt;
        PD_MSG_FIELD_I(properties) = 0;
        PD_MSG_FIELD_I(satisfierGuid.guid) = base->guid;
        PD_MSG_FIELD_I(satisfierGuid.metaDataPtr) = base;
        for(i = 0; i < event->base.waitersCount; ++i) {
#ifdef OCR_ENABLE_STATISTICS
            // We do this *before* calling signalWaiter because otherwise
            // if the event is a OCR_EVENT_ONCE_T, it will get freed
            // before we can get the message sent
            statsDEP_SATISFYFromEvt(pd, base->guid, base, waiter->guid,
                                    data.guid, waiter->slot);
#endif
            DPRINTF(DEBUG_LVL_INFO, "SatisfyFromEvent %s: 0x%lx with 0x%lx\n",
                    eventTypeToString(base), waiters[i].guid, base->guid);
            getCurrentEnv(NULL, NULL, NULL, &msg);
            msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
            PD_MSG_FIELD_I(guid.guid) = waiters[i].guid;
            PD_MSG_FIELD_I(guid.metaDataPtr) = NULL;
            PD_MSG_FIELD_I(slot) = waiters[i].slot;
            RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
        }
        // Release the DB
#undef PD_TYPE
#define PD_TYPE PD_MSG_DB_RELEASE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
        PD_MSG_FIELD_I(edt) = currentEdt;
        PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE
    }
    // The latch is satisfied so we destroy it
    return destructEventHc(base);
}

u8 registerSignalerHc(ocrEvent_t *self, ocrFatGuid_t signaler, u32 slot,
                      ocrDbAccessMode_t mode, bool isDepAdd) {
    return 0; // We do not do anything for signalers
}

u8 unregisterSignalerHc(ocrEvent_t *self, ocrFatGuid_t signaler, u32 slot,
                        bool isDepRem) {
    return 0; // We do not do anything for signalers
}

/**
 * In this call, we do not contend with the satisfy (once and latch events) however,
 * we do contend with multiple registration.
 * By construction, users must ensure a ONCE event is registered before satisfy is called.
 */
u8 registerWaiterEventHc(ocrEvent_t *base, ocrFatGuid_t waiter, u32 slot, bool isDepAdd) {
    // Here we always add the waiter to our list so we ignore isDepAdd
    ocrEventHc_t *event = (ocrEventHc_t*)base;

    DPRINTF(DEBUG_LVL_INFO, "Register waiter %s: 0x%lx with waiter 0x%lx on slot %d\n",
            eventTypeToString(base), base->guid, waiter.guid, slot);

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    regNode_t *waiters = NULL;
    ocrFatGuid_t newGuid = {.guid = NULL_GUID, .metaDataPtr = NULL};
    regNode_t *waitersNew = NULL;
    u32 i;
    getCurrentEnv(&pd, NULL, &curTask, &msg);
    //TODO shouldn't this stuff be protected by a lock for latch/once ??
    // Acquire the DB that contains the waiters
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->waitersDb;
    PD_MSG_FIELD_IO(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_IO(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_IO(properties) = DB_MODE_ITW | DB_PROP_RT_ACQUIRE;
    //Should be a local DB
    u8 res = pd->fcts.processMessage(pd, &msg, true);
    ASSERT(!res);
    waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
    // TODO: See bug #273
    event->waitersDb = PD_MSG_FIELD_IO(guid);
    ASSERT(waiters);
#undef PD_TYPE
    if((event->waitersCount + 1) == event->waitersMax) {
        // We need to create a new DB
#define PD_TYPE PD_MSG_DB_CREATE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->waitersDb;
        PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
        PD_MSG_FIELD_IO(size) = sizeof(regNode_t)*event->waitersMax*2;
        PD_MSG_FIELD_I(affinity.guid) = NULL_GUID;
        PD_MSG_FIELD_I(affinity.metaDataPtr) = NULL;
        PD_MSG_FIELD_IO(properties) = 0;
        PD_MSG_FIELD_I(dbType) = RUNTIME_DBTYPE;
        PD_MSG_FIELD_I(allocator) = NO_ALLOC;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));

        waitersNew = (regNode_t*)PD_MSG_FIELD_O(ptr);
        newGuid = PD_MSG_FIELD_IO(guid);
#undef PD_TYPE
        hal_memCopy(waitersNew, waiters, sizeof(regNode_t)*event->waitersCount, false);
        event->waitersMax *= 2;
        for(i = event->waitersCount; i < event->waitersMax; ++i) {
            waitersNew[i].guid = NULL_GUID;
            waitersNew[i].slot = 0;
            waitersNew[i].mode = -1;
        }
        waiters = waitersNew;
    }

    waiters[event->waitersCount].guid = waiter.guid;
    waiters[event->waitersCount].slot = slot;
    ++event->waitersCount;

    // Now release the DB(s)
    if(waitersNew) {
        // We need to destroy the old DB and release the new one
#define PD_TYPE PD_MSG_DB_FREE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_FREE | PD_MSG_REQUEST;
        PD_MSG_FIELD_I(guid) = event->waitersDb;
        PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
        PD_MSG_FIELD_I(properties) = 0;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, false));
        event->waitersDb = newGuid;
#undef PD_TYPE
    }

    // hal_unlock32(&(event->waitersLock));

    // We always release waitersDb as it has been set properly if needed
#define PD_TYPE PD_MSG_DB_RELEASE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_I(properties) = waitersNew?0:DB_PROP_RT_ACQUIRE;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE

    return 0;
}


/**
 * @brief Registers waiters on persistent events such as sticky or idempotent.
 *
 * This code contends with a satisfy call and with concurrent add-dependences that try
 * to register their waiter.
 * The event waiterLock is grabbed, if the event is already satisfied, directly satisfy
 * the waiter. Otherwise add the waiter's guid to the waiters db list. If db is too small
 * reallocate and copy over to a new one.
 *
 * Returns non-zero if the registerWaiter requires registerSignaler to be called there-after
 */
u8 registerWaiterEventHcPersist(ocrEvent_t *base, ocrFatGuid_t waiter, u32 slot, bool isDepAdd) {
    ocrEventHcPersist_t *event = (ocrEventHcPersist_t*)base;

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    regNode_t *waiters = NULL;
    ocrFatGuid_t newGuid = {.guid = NULL_GUID, .metaDataPtr = NULL};
    regNode_t *waitersNew = NULL;
    u32 i;
    u8 toReturn = 0;
    getCurrentEnv(&pd, NULL, &curTask, &msg);
    ocrFatGuid_t currentEdt;
    currentEdt.guid = (curTask == NULL) ? NULL_GUID : curTask->guid;
    currentEdt.metaDataPtr = curTask;

    // EDTs incrementally register on their dependences as elements
    // get satisfied (Guarantees O(n) traversal of dependence list).
    // Other events register when the dependence is added
    ocrGuidKind waiterKind = OCR_GUID_NONE;
    RESULT_ASSERT(guidKind(pd, waiter, &waiterKind), ==, 0);

    if(isDepAdd && waiterKind == OCR_GUID_EDT) {
        // If we're adding a dependence and the waiter is an EDT we
        // skip this part. The event is registered on the EDT and
        // the EDT will register on the event only when its dependence
        // frontier reaches this event.
        return 0; //Require registerSignaler invocation
    }
    ASSERT(waiterKind == OCR_GUID_EDT || (waiterKind & OCR_GUID_EVENT));

    DPRINTF(DEBUG_LVL_INFO, "Register waiter %s: 0x%lx with waiter 0x%lx on slot %d\n",
            eventTypeToString(base), base->guid, waiter.guid, slot);
    hal_lock32(&(event->waitersLock));
    if(event->data != UNINITIALIZED_GUID) {
        hal_unlock32(&(event->waitersLock));
        // We send a message saying that we satisfy whatever tried to wait on us
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
        msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
        PD_MSG_FIELD_I(guid) = waiter;
        PD_MSG_FIELD_I(payload.guid) = event->data;
        PD_MSG_FIELD_I(payload.metaDataPtr) = NULL;
        PD_MSG_FIELD_I(currentEdt) = currentEdt;
        PD_MSG_FIELD_I(satisfierGuid.guid) = base->guid;
        PD_MSG_FIELD_I(satisfierGuid.metaDataPtr) = base;
        PD_MSG_FIELD_I(slot) = slot;
        PD_MSG_FIELD_I(properties) = 0;
        if((toReturn = pd->fcts.processMessage(pd, &msg, false))) {
            return toReturn; //TODO error status
        }
#undef PD_MSG
#undef PD_TYPE
        return 0; //Require registerSignaler invocation
    }

    // Here we need to actually update our waiter list. We still hold the lock
    // Acquire the DB that contains the waiters
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
    PD_MSG_FIELD_IO(edt) = currentEdt;
    PD_MSG_FIELD_IO(properties) = DB_MODE_ITW | DB_PROP_RT_ACQUIRE;
    //Should be a local DB
    if((toReturn = pd->fcts.processMessage(pd, &msg, true))) {
        // should be the only writer active on the waiter DB since we have the lock
        ASSERT(false); // debug
        ASSERT(toReturn != OCR_EBUSY);
        hal_unlock32(&(event->waitersLock));
        return toReturn; //TODO error status
    }

    waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
    // TODO: See bug #273
    event->base.waitersDb = PD_MSG_FIELD_IO(guid);
    ASSERT(waiters);
#undef PD_TYPE
    if(event->base.waitersCount + 1 == event->base.waitersMax) {
        // We need to create a new DB
#define PD_TYPE PD_MSG_DB_CREATE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
        PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
        PD_MSG_FIELD_IO(size) = sizeof(regNode_t)*event->base.waitersMax*2;
        PD_MSG_FIELD_I(affinity.guid) = NULL_GUID;
        PD_MSG_FIELD_I(affinity.metaDataPtr) = NULL;
        PD_MSG_FIELD_IO(properties) = 0;
        PD_MSG_FIELD_I(dbType) = RUNTIME_DBTYPE;
        PD_MSG_FIELD_I(allocator) = NO_ALLOC;
        if((toReturn = pd->fcts.processMessage(pd, &msg, true))) {
            ASSERT(false); // debug
            hal_unlock32(&(event->waitersLock));
            return toReturn; //TODO error status
        }

        waitersNew = (regNode_t*)PD_MSG_FIELD_O(ptr);
        newGuid = PD_MSG_FIELD_IO(guid);
#undef PD_TYPE
        hal_memCopy(waitersNew, waiters, sizeof(regNode_t)*event->base.waitersCount, false);
        event->base.waitersMax *= 2;
        for(i = event->base.waitersCount; i < event->base.waitersMax; ++i) {
            waitersNew[i].guid = NULL_GUID;
            waitersNew[i].slot = 0;
            waitersNew[i].mode = -1;
        }
        waiters = waitersNew;
    }
    waiters[event->base.waitersCount].guid = waiter.guid;
    waiters[event->base.waitersCount].slot = slot;

    ++event->base.waitersCount;
    // Now release the DB(s)
    if(waitersNew) {
        // We need to release and destroy the old DB and release the new one
#define PD_TYPE PD_MSG_DB_RELEASE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
        PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
        PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
        RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_TYPE
#define PD_TYPE PD_MSG_DB_FREE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DB_FREE | PD_MSG_REQUEST;
        PD_MSG_FIELD_I(guid) = event->base.waitersDb;
        PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
        PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
        PD_MSG_FIELD_I(properties) = 0;
        if((toReturn = pd->fcts.processMessage(pd, &msg, false))) {
            ASSERT(false); // debug
            hal_unlock32(&(event->waitersLock));
            return toReturn; //TODO error status
        }
#undef PD_TYPE
        event->base.waitersDb = newGuid;
    }
    // We can release the lock now
    hal_unlock32(&(event->waitersLock));

    // We always release waitersDb as it has been set properly if needed
#define PD_TYPE PD_MSG_DB_RELEASE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE
    return 0; //Require registerSignaler invocation
}

// In this call, we do not contend with satisfy
u8 unregisterWaiterEventHc(ocrEvent_t *base, ocrFatGuid_t waiter, u32 slot, bool isDepRem) {
    // Always search for the waiter because we don't know if it registered or not so
    // ignore isDepRem
    ocrEventHc_t *event = (ocrEventHc_t*)base;


    DPRINTF(DEBUG_LVL_INFO, "UnRegister waiter %s: 0x%lx with waiter 0x%lx on slot %d\n",
            eventTypeToString(base), base->guid, waiter.guid, slot);

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    regNode_t *waiters = NULL;
    u32 i;

    getCurrentEnv(&pd, NULL, &curTask, &msg);

    // Acquire the DB that contains the waiters
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->waitersDb;
    PD_MSG_FIELD_IO(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_IO(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_IO(properties) = DB_MODE_ITW | DB_PROP_RT_ACQUIRE;
    //Should be a local DB
    u8 res = pd->fcts.processMessage(pd, &msg, true);
    ASSERT(!res);

    waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
    // TODO: Bug #273
    event->waitersDb = PD_MSG_FIELD_IO(guid);
    ASSERT(waiters);
#undef PD_TYPE
    // We search for the waiter that we need and remove it
    for(i = 0; i < event->waitersCount; ++i) {
        if(waiters[i].guid == waiter.guid && waiters[i].slot == slot) {
            // We will copy all the other ones
            hal_memCopy((void*)&waiters[i], (void*)&waiters[i+1],
                        sizeof(regNode_t)*(event->waitersCount - i - 1), false);
            --event->waitersCount;
            break;
        }
    }

    // We always release waitersDb
#define PD_TYPE PD_MSG_DB_RELEASE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}


// In this call, we can have concurrent satisfy
u8 unregisterWaiterEventHcPersist(ocrEvent_t *base, ocrFatGuid_t waiter, u32 slot) {
    ocrEventHcPersist_t *event = (ocrEventHcPersist_t*)base;


    DPRINTF(DEBUG_LVL_INFO, "Unregister waiter %s: 0x%lx with waiter 0x%lx on slot %d\n",
            eventTypeToString(base), base->guid, waiter.guid, slot);

    ocrPolicyDomain_t *pd = NULL;
    ocrTask_t *curTask = NULL;
    PD_MSG_STACK(msg);
    regNode_t *waiters = NULL;
    u32 i;
    u8 toReturn = 0;

    getCurrentEnv(&pd, NULL, &curTask, &msg);
    hal_lock32(&(event->waitersLock));
    if(event->data != UNINITIALIZED_GUID) {
        // We don't really care at this point so we don't do anything
        hal_unlock32(&(event->waitersLock));
        return 0;
    }

    // Here we need to actually update our waiter list. We still hold the lock
    // Acquire the DB that contains the waiters
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_ACQUIRE
    msg.type = PD_MSG_DB_ACQUIRE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
    PD_MSG_FIELD_IO(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_IO(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_IO(properties) = DB_MODE_ITW | DB_PROP_RT_ACQUIRE;
    //Should be a local DB
    if((toReturn = pd->fcts.processMessage(pd, &msg, true))) {
        ASSERT(!toReturn);
        hal_unlock32(&(event->waitersLock));
        return toReturn;
    }
    // TODO: Guid reading (bug #273)
    waiters = (regNode_t*)PD_MSG_FIELD_O(ptr);
    event->base.waitersDb = PD_MSG_FIELD_IO(guid);
    ASSERT(waiters);
#undef PD_TYPE
    // We search for the waiter that we need and remove it
    for(i = 0; i < event->base.waitersCount; ++i) {
        if(waiters[i].guid == waiter.guid && waiters[i].slot == slot) {
            // We will copy all the other ones
            hal_memCopy((void*)&waiters[i], (void*)&waiters[i+1],
                        sizeof(regNode_t)*(event->base.waitersCount - i - 1), false);
            --event->base.waitersCount;
            break;
        }
    }

    // We can release the lock now
    hal_unlock32(&(event->waitersLock));

    // We always release waitersDb
#define PD_TYPE PD_MSG_DB_RELEASE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->base.waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_I(properties) = DB_PROP_RT_ACQUIRE;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

/******************************************************/
/* OCR-HC Events Factory                              */
/******************************************************/

// takesArg indicates whether or not this event carries any data
ocrEvent_t * newEventHc(ocrEventFactory_t * factory, ocrEventTypes_t eventType,
                        bool takesArg, ocrParamList_t *perInstance) {

    // Get the current environment
    ocrPolicyDomain_t *pd = NULL;
    PD_MSG_STACK(msg);
    ocrTask_t *curTask = NULL;
    u32 i;

    getCurrentEnv(&pd, NULL, &curTask, &msg);

    // Create the event itself by getting a GUID
    u64 sizeOfGuid = sizeof(ocrEventHc_t);
    if(eventType == OCR_EVENT_LATCH_T) {
        sizeOfGuid = sizeof(ocrEventHcLatch_t);
    }
    if((eventType == OCR_EVENT_IDEM_T) || (eventType == OCR_EVENT_STICKY_T)) {
        sizeOfGuid = sizeof(ocrEventHcPersist_t);
    }

    ocrGuidKind kind;
    switch(eventType) {
        case OCR_EVENT_ONCE_T:
            kind = OCR_GUID_EVENT_ONCE;
            break;
        case OCR_EVENT_IDEM_T:
            kind = OCR_GUID_EVENT_IDEM;
            break;
        case OCR_EVENT_STICKY_T:
            kind = OCR_GUID_EVENT_STICKY;
            break;
        case OCR_EVENT_LATCH_T:
            kind = OCR_GUID_EVENT_LATCH;
            break;
        default:
            kind = OCR_GUID_NONE; // To keep clang happy
            ASSERT(false && "Unknown type of event");
    }

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE
    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid.guid) = UNINITIALIZED_GUID;
    PD_MSG_FIELD_IO(guid.metaDataPtr) = NULL;
    // We allocate everything in the meta-data to keep things simple
    PD_MSG_FIELD_I(size) = sizeOfGuid;
    PD_MSG_FIELD_I(kind) = kind;
    PD_MSG_FIELD_I(properties) = 0;
    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), NULL);
    ocrEventHc_t *event = (ocrEventHc_t*)PD_MSG_FIELD_IO(guid.metaDataPtr);
    ocrEvent_t *base = (ocrEvent_t*)event;
    ASSERT(event);

    // Set-up base structures
    base->guid = PD_MSG_FIELD_IO(guid.guid);
    base->kind = eventType;
    base->fctId = factory->factoryId;
#undef PD_MSG
#undef PD_TYPE

    // Set-up HC specific structures
    event->waitersCount = event->signalersCount = 0;
    event->waitersMax = INIT_WAITER_COUNT;
    event->signalersMax = INIT_SIGNALER_COUNT;
    if(eventType == OCR_EVENT_LATCH_T) {
        // Initialize the counter
        ((ocrEventHcLatch_t*)event)->counter = 0;
    }
    if(eventType == OCR_EVENT_IDEM_T || eventType == OCR_EVENT_STICKY_T) {
        ((ocrEventHcPersist_t*)event)->data = UNINITIALIZED_GUID;
        ((ocrEventHcPersist_t*)event)->waitersLock = 0;
    }

    // Now we need to get the GUIDs for the waiters and
    // signalers data-blocks
    event->waitersDb.guid = UNINITIALIZED_GUID;
    event->waitersDb.metaDataPtr = NULL;

    event->signalersDb.guid = UNINITIALIZED_GUID;
    event->signalersDb.metaDataPtr = NULL;

    regNode_t *temp = NULL;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_CREATE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_IO(size) = sizeof(regNode_t)*INIT_WAITER_COUNT;
    PD_MSG_FIELD_I(affinity.guid) = NULL_GUID;
    PD_MSG_FIELD_I(affinity.metaDataPtr) = NULL_GUID;
    PD_MSG_FIELD_IO(properties) = 0;
    PD_MSG_FIELD_I(dbType) = RUNTIME_DBTYPE;
    PD_MSG_FIELD_I(allocator) = NO_ALLOC;
    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), NULL);
    event->waitersDb = PD_MSG_FIELD_IO(guid);
    temp = (regNode_t*)PD_MSG_FIELD_O(ptr);
    event->waitersDb = PD_MSG_FIELD_IO(guid);
    for(i = 0; i < INIT_WAITER_COUNT; ++i) {
        temp[i].guid = UNINITIALIZED_GUID;
        temp[i].slot = 0;
        temp[i].mode = -1;
    }

    /* Signalers not used at this point
    // TODO: FIXME when reuse of message function implemented
    // We probably need to just call something to reset a bit
    msg.type = PD_MSG_DB_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->signalersDb;
    PD_MSG_FIELD_IO(size) = sizeof(regNode_t)*INIT_SIGNALER_COUNT;
    RESULT_PROPAGATE(pd->fcts.processMessage(pd, &msg, true));
    event->signalersDb = PD_MSG_FIELD_IO(guid);
    temp = (regNode_t*)PD_MSG_FIELD_O(ptr);
    for(i = 0; i < INIT_SIGNALER_COUNT; ++i) {
        temp[i].guid = UNINITIALIZED_GUID;
        temp[i].slot = 0;
    }
    */
#undef PD_TYPE
#define PD_TYPE PD_MSG_DB_RELEASE
    getCurrentEnv(NULL, NULL, NULL, &msg);
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid) = event->waitersDb;
    PD_MSG_FIELD_I(edt.guid) = curTask ? curTask->guid : NULL_GUID;
    PD_MSG_FIELD_I(edt.metaDataPtr) = curTask;
    PD_MSG_FIELD_I(properties) = 0;
    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, true), NULL);
#undef PD_MSG
#undef PD_TYPE

    DPRINTF(DEBUG_LVL_INFO, "Create %s: 0x%lx\n", eventTypeToString(base), base->guid);
#ifdef OCR_ENABLE_STATISTICS
    statsEVT_CREATE(getCurrentPD(), getCurrentEDT(), NULL, base->guid, base);
#endif
    return base;
}

void destructEventFactoryHc(ocrEventFactory_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

ocrEventFactory_t * newEventFactoryHc(ocrParamList_t *perType, u32 factoryId) {
    ocrEventFactory_t* base = (ocrEventFactory_t*) runtimeChunkAlloc(
                                  sizeof(ocrEventFactoryHc_t), NULL);

    base->instantiate = FUNC_ADDR(ocrEvent_t* (*)(ocrEventFactory_t*,
                                  ocrEventTypes_t, bool, ocrParamList_t*), newEventHc);
    base->destruct =  FUNC_ADDR(void (*)(ocrEventFactory_t*), destructEventFactoryHc);
    // Initialize the function pointers
    u32 i;
    // Setup functions properly
    for(i = 0; i < (u32)OCR_EVENT_T_MAX; ++i) {
        base->fcts[i].destruct = FUNC_ADDR(u8 (*)(ocrEvent_t*), destructEventHc);
        base->fcts[i].get = FUNC_ADDR(ocrFatGuid_t (*)(ocrEvent_t*), getEventHc);
        base->fcts[i].registerSignaler = FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32, ocrDbAccessMode_t, bool), registerSignalerHc);
        base->fcts[i].unregisterSignaler = FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32, bool), unregisterSignalerHc);
    }
    // Setup satisfy function pointers
    base->fcts[OCR_EVENT_ONCE_T].satisfy =
        FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32), satisfyEventHcOnce);
    base->fcts[OCR_EVENT_LATCH_T].satisfy =
        FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32), satisfyEventHcLatch);
    base->fcts[OCR_EVENT_IDEM_T].satisfy =
    base->fcts[OCR_EVENT_STICKY_T].satisfy =
        FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32), satisfyEventHcPersist);

    // Setup registration function pointers
    base->fcts[OCR_EVENT_ONCE_T].registerWaiter =
    base->fcts[OCR_EVENT_LATCH_T].registerWaiter =
         FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32, bool), registerWaiterEventHc);
    base->fcts[OCR_EVENT_IDEM_T].registerWaiter =
    base->fcts[OCR_EVENT_STICKY_T].registerWaiter =
        FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32, bool), registerWaiterEventHcPersist);

    base->fcts[OCR_EVENT_ONCE_T].unregisterWaiter =
    base->fcts[OCR_EVENT_LATCH_T].unregisterWaiter =
        FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32, bool), unregisterWaiterEventHc);
    base->fcts[OCR_EVENT_IDEM_T].unregisterWaiter =
    base->fcts[OCR_EVENT_STICKY_T].unregisterWaiter =
        FUNC_ADDR(u8 (*)(ocrEvent_t*, ocrFatGuid_t, u32, bool), unregisterWaiterEventHcPersist);

    base->factoryId = factoryId;

    return base;
}
#endif /* ENABLE_EVENT_HC */
