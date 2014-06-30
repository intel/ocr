/**
* @brief Data-block implementation for OCR
*/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-allocator.h"
#include "ocr-datablock.h"
#include "ocr-db.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"

//#if (__STDC_HOSTED__ == 1)
//#include <string.h>
//#endif

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "utils/profiler/profiler.h"

#define DEBUG_TYPE API

u8 ocrDbCreate(ocrGuid_t *db, void** addr, u64 len, u16 flags,
               ocrGuid_t affinity, ocrInDbAllocator_t allocator) {

    START_PROFILE(api_DbCreate);
    // TODO: Currently location and allocator are ignored
    // ocrDataBlock_t *createdDb = newDataBlock(OCR_DATABLOCK_DEFAULT);
    // ocrDataBlock_t *createdDb = newDataBlock(OCR_DATABLOCK_PLACED);

    // TODO: I need to get the current policy to figure out my allocator.
    // Replace with allocator that is gotten from policy
    //
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    ocrTask_t *task = NULL;
    u8 returnCode = 0;
    getCurrentEnv(&policy, NULL, &task, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_CREATE
    msg.type = PD_MSG_DB_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = *db;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(edt.guid) = task?task->guid:NULL_GUID; // Can happen when non EDT creates the DB
    PD_MSG_FIELD(edt.metaDataPtr) = task;
    PD_MSG_FIELD(size) = len;
    PD_MSG_FIELD(affinity.guid) = affinity;
    PD_MSG_FIELD(affinity.metaDataPtr) = NULL;
    PD_MSG_FIELD(properties) = (u32)flags;
    PD_MSG_FIELD(dbType) = USER_DBTYPE;
    PD_MSG_FIELD(allocator) = allocator;

    returnCode = policy->fcts.processMessage(policy, &msg, true);
    if(returnCode == 0) {
        *db = PD_MSG_FIELD(guid.guid);
        *addr = PD_MSG_FIELD(ptr);
    } else {
        *addr = NULL;
    }
    if(*addr == NULL) {
        RETURN_PROFILE(OCR_ENOMEM);
    }
#undef PD_MSG
#undef PD_TYPE

    if(task) {
        // Here we inform the task that we created a DB
        // This is most likely ALWAYS a local message but let's leave the
        // API as it is for now. It is possible that the EDTs move at some point so
        // just to be safe
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_DYNADD
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DEP_DYNADD | PD_MSG_REQUEST;
        PD_MSG_FIELD(edt.guid) = task->guid;
        PD_MSG_FIELD(edt.metaDataPtr) = task;
        PD_MSG_FIELD(db.guid) = *db;
        PD_MSG_FIELD(db.metaDataPtr) = NULL;
        PD_MSG_FIELD(properties) = 0;
        policy->fcts.processMessage(policy, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    } else {
        DPRINTF(DEBUG_LVL_WARN, "Acquiring DB (GUID: 0x%lx) from outside an EDT ... auto-release will fail\n",
                *db);
    }
    RETURN_PROFILE(0);
}

u8 ocrDbDestroy(ocrGuid_t db) {

    START_PROFILE(api_DbDestroy);
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    ocrTask_t *task = NULL;
    getCurrentEnv(&policy, NULL, &task, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_FREE
    msg.type = PD_MSG_DB_FREE | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = db;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(edt.guid) = task?task->guid:NULL_GUID;
    PD_MSG_FIELD(edt.metaDataPtr) = task;
    PD_MSG_FIELD(properties) = 0;

    u8 returnCode =  policy->fcts.processMessage(policy, &msg, false);
#undef PD_MSG
#undef PD_TYPE

    if(task) {
        // Here we inform the task that we destroyed (and therefore released) a DB
        // This is most likely ALWAYS a local message but let's leave the
        // API as it is for now. It is possible that the EDTs move at some point so
        // just to be safe
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_DYNREMOVE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DEP_DYNREMOVE | PD_MSG_REQUEST;
        PD_MSG_FIELD(edt.guid) = task->guid;
        PD_MSG_FIELD(edt.metaDataPtr) = task;
        PD_MSG_FIELD(db.guid) = db;
        PD_MSG_FIELD(db.metaDataPtr) = NULL;
        PD_MSG_FIELD(properties) = 0;
        policy->fcts.processMessage(policy, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    } else {
        DPRINTF(DEBUG_LVL_WARN, "Destroying DB (GUID: 0x%lx) from outside an EDT ... auto-release will fail\n",
                db);
    }
    RETURN_PROFILE(returnCode);
}

u8 ocrDbRelease(ocrGuid_t db) {

    START_PROFILE(api_DbRelease);
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *policy = NULL;
    ocrTask_t *task = NULL;
    getCurrentEnv(&policy, NULL, &task, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DB_RELEASE
    msg.type = PD_MSG_DB_RELEASE | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = db;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(edt.guid) = task?task->guid:NULL_GUID;
    PD_MSG_FIELD(edt.metaDataPtr) = task;
    PD_MSG_FIELD(properties) = 0;

    u8 returnCode = policy->fcts.processMessage(policy, &msg, false);
#undef PD_MSG
#undef PD_TYPE

    if(task) {
        // Here we inform the task that we released a DB
        // This is most likely ALWAYS a local message but let's leave the
        // API as it is for now. It is possible that the EDTs move at some point so
        // just to be safe
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_DYNREMOVE
        getCurrentEnv(NULL, NULL, NULL, &msg);
        msg.type = PD_MSG_DEP_DYNREMOVE | PD_MSG_REQUEST;
        PD_MSG_FIELD(edt.guid) = task->guid;
        PD_MSG_FIELD(edt.metaDataPtr) = task;
        PD_MSG_FIELD(db.guid) = db;
        PD_MSG_FIELD(db.metaDataPtr) = NULL;
        PD_MSG_FIELD(properties) = 0;
        policy->fcts.processMessage(policy, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    } else {
        DPRINTF(DEBUG_LVL_WARN, "Releasing DB (GUID: 0x%lx) from outside an EDT ... auto-release will fail\n",
                db);
    }
    RETURN_PROFILE(returnCode);
}

u8 ocrDbMalloc(ocrGuid_t guid, u64 size, void** addr) {
    return OCR_EINVAL; /* not yet implemented */
}

u8 ocrDbMallocOffset(ocrGuid_t guid, u64 size, u64* offset) {
    return OCR_EINVAL; /* not yet implemented */
}

struct ocrDbCopy_args {
    ocrGuid_t destination;
    u64 destinationOffset;
    ocrGuid_t source;
    u64 sourceOffset;
    u64 size;
} ocrDbCopy_args;

// TODO: Re-enable
/*
ocrGuid_t ocrDbCopy_edt ( u32 paramc, u64 * params, void* paramv[], u32 depc, ocrEdtDep_t depv[]) {
    char *sptr, *dptr;
    struct ocrDbCopy_args * pv = (struct ocrDbCopy_args *) depv[0].ptr;
    ocrGuid_t destination = pv->destination;
    u64 destinationOffset = pv->destinationOffset;
    ocrGuid_t source = pv->source;
    u64 sourceOffset = pv->sourceOffset;
    u64 size = pv->size;

    ocrDbAcquire(source, (void *) &sptr, 0);
    ocrDbAcquire(destination, (void *) &dptr, 0);

    sptr += sourceOffset;
    dptr += destinationOffset;

#if (__STDC_HOSTED__ == 1)
    memcpy((void *)dptr, (const void *)sptr, size);
#else
    int i;
    for (i = 0; i < size; i++) {
        dptr[i] = sptr[i];
    }
#endif

    ocrDbRelease(source);
    ocrDbRelease(destination);

    ocrGuid_t paramDbGuid = (ocrGuid_t)depv[0].guid;
    ocrDbDestroy(paramDbGuid);

    return NULL_GUID;
}
*/

u8 ocrDbCopy(ocrGuid_t destination, u64 destinationOffset, ocrGuid_t source,
             u64 sourceOffset, u64 size, u64 copyType, ocrGuid_t *completionEvt) {
    ASSERT(0);

    // // Create the event
    // ocrGuid_t eventGuid;
    // ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, TRUE); /*TODO: Replace with ONCE after that is supported */

    // // Create the EDT
    // ocrGuid_t edtGuid;
    // ocrEdtCreate(&edtGuid, ocrDbCopy_edt, 0, NULL, NULL, 0, 1, &eventGuid, completionEvt);
    // ocrEdtSchedule(edtGuid);

    // // Create the copy params
    // ocrGuid_t paramDbGuid;
    // struct ocrDbCopy_args * dbArgs = NULL;
    // void * ptr = (void *) dbArgs;
    // // Warning: directly casting dbArgs to (void **) causes a type-punning warning with gcc-4.1.2
    // ocrDbCreate(&paramDbGuid, &ptr, sizeof(ocrDbCopy_args), 0xdead, NULL, NO_ALLOC);

    // dbArgs->destination = destination;
    // dbArgs->destinationOffset = destinationOffset;
    // dbArgs->source = source;
    // dbArgs->sourceOffset = sourceOffset;
    // dbArgs->size = size;

    // ocrEventSatisfy(eventGuid, paramDbGuid);

    // /* ocrDbRelease(paramDbGuid); TODO: BUG: Release tries to free */

    return 0;
}

u8 ocrDbFree(ocrGuid_t guid, void* addr) {
    return OCR_EINVAL; /* not yet implemented */
}

u8 ocrDbFreeOffset(ocrGuid_t guid, u64 offset) {
    return OCR_EINVAL; /* not yet implemented */
}
