/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sal.h"
#include "ocr-types.h"

static void ocrShutdownInternal(u8 errorCode) {
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    ocrPolicyMsg_t * msgPtr = &msg;
    ocrTask_t * curEdt = NULL;
    getCurrentEnv(&pd, NULL, &curEdt, msgPtr);
#define PD_MSG msgPtr
#define PD_TYPE PD_MSG_MGT_SHUTDOWN
    msgPtr->type = PD_MSG_MGT_SHUTDOWN | PD_MSG_REQUEST;
    PD_MSG_FIELD(errorCode) = errorCode;
    PD_MSG_FIELD(currentEdt.guid) = curEdt ? curEdt->guid : NULL_GUID;
    PD_MSG_FIELD(currentEdt.metaDataPtr) = curEdt;
    RESULT_ASSERT(pd->fcts.processMessage(pd, msgPtr, true), ==, 0);
#undef PD_MSG
#undef PD_TYPE
    // TODO: Re-enable teardown for other platforms
    //teardown(pd);
}

void ocrShutdown() {
    ocrShutdownInternal(0);
}

void ocrAbort(u8 errorCode) {
    ocrShutdownInternal(errorCode);
}

u64 getArgc(void* dbPtr) {
    return ((u64*)dbPtr)[0];
}

char* getArgv(void* dbPtr, u64 count) {
    u64* dbPtrAsU64 = (u64*)dbPtr;
    ASSERT(count < dbPtrAsU64[0]);
    u64 offset = dbPtrAsU64[1 + count];
    return ((char*)dbPtr) + offset;
}
