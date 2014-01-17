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
    getCurrentEnv(&pd, NULL, NULL, NULL);
    pd->stop(pd);
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
