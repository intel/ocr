/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - test querying the affinity of a NULL_GUID
 */

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t currentAffinity;
    ocrAffinityGetCurrent(&currentAffinity);
    ocrGuid_t queriedDbAffinityGuid;
    u64 count = 1;
    ocrAffinityQuery(NULL_GUID, &count, &queriedDbAffinityGuid);
    ASSERT(queriedDbAffinityGuid == currentAffinity);
    ocrShutdown();
    return NULL_GUID;
}
