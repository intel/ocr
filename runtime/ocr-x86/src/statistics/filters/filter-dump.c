/**
* @brief A dump filter that dumps all information to a file
*/

/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#ifdef OCR_ENABLE_STATISTICS

#define FILTER_NAME FILE_DUMP

#include "filters-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// A simple filter that dumps everything to a file

START_FILTER
ocrLock_t *lock;
FILE *outFile;
END_FILTER

FILTER_DESTRUCT {
FILTER_CAST(rself, self);
DPRINTF(DEBUG_LVL_VERB, "Destroying file-dump filter @ 0x%lx\n", (u64)self);
// We close out the output file
if(rself->outFile) {
    fclose(rself->outFile);
}
// Also destroy the lock
rself->lock->fctPtrs->destruct(rself->lock);

// Destroy ourself as well
free(self);
}

FILTER_DUMP {
ASSERT(0); // Should never be called
return 0;
}

FILTER_NOTIFY {
ASSERT(0); // Should never be called
}

FILTER_MERGE {
FILTER_CAST(rself, self);
DPRINTF(DEBUG_LVL_VERB, "Filter @ 0x%lx received merging from filter 0x%lx\n", (u64)self, (u64)other);

if(rself->outFile) {
    char* outString;
    u64 nextChunk = 0;
    rself->lock->fctPtrs->lock(rself->lock);
        do {
            nextChunk = other->fcts.dump(other, &outString, nextChunk, NULL);
            if(outString) {
                fprintf(rself->outFile, "%s\n", outString);
                free(outString);
            }
        } while(nextChunk);
        rself->lock->fctPtrs->unlock(rself->lock);
    }

    if(toKill) {
        other->parent = NULL;
        other->fcts.destruct(other);
    }
}

FILTER_CREATE {
    FILTER_MALLOC(rself);
    FILTER_SETUP(rself, parent);

    ASSERT(parent == NULL);
    rself->base.parent = NULL;

    ocrPolicyDomain_t *pd = getCurrentPD();
    ocrPolicyCtx_t *ctx = getCurrentWorkerContext();
    rself->lock = pd->getLock(pd, ctx);

    // Open the file
    if(instanceArg)
        rself->outFile = fopen((char*)instanceArg, "w");
    else
        rself->outFile = fopen("ocrStats.log", "w");
    DPRINTF(DEBUG_LVL_VERB, "Created a file-dump filter @ 0x%lx with file 0x%lx\n",
            (u64)rself, (u64)rself->outFile);
    ASSERT(rself->outFile);
    
    return (ocrStatsFilter_t*)rself;
}

#endif /* OCR_ENABLE_STATISTICS */
