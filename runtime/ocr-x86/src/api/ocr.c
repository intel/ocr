/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sal.h"
#include "ocr-types.h"

void ocrShutdown() {
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    ocrPolicyMsg_t * msgPtr = &msg;
    ocrTask_t * curEdt = NULL;
    getCurrentEnv(&pd, NULL, &curEdt, msgPtr);
#define PD_MSG msgPtr
#define PD_TYPE PD_MSG_MGT_SHUTDOWN
    msgPtr->type = PD_MSG_MGT_SHUTDOWN | PD_MSG_REQUEST;
    PD_MSG_FIELD(currentEdt.guid) = curEdt ? curEdt->guid : NULL_GUID;
    PD_MSG_FIELD(currentEdt.metaDataPtr) = curEdt;
    RESULT_ASSERT(pd->fcts.processMessage(pd, msgPtr, true), ==, 0);
#undef PD_MSG
#undef PD_TYPE
    // TODO: Re-enable teardown for other platforms
    //teardown(pd);
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
