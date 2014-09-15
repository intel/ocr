/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create a remote EDT with EDT_PARAM_UNK paramc (+ a lot more for the sink EDT)
 */

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ASSERT(paramc == 4);
    ASSERT(paramv[0] == 111);
    ASSERT(paramv[1] == 222);
    ASSERT(paramv[2] == 333);
    ASSERT(paramv[3] == 444);
    PRINTF("[remote] RemoteEdt: 4 paramv checked\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t edtAffinity = affinities[affinityCount-1];

    // Setup the test
    ocrGuid_t remoteEdtTemplateGuid;
    ocrEdtTemplateCreate(&remoteEdtTemplateGuid, remoteEdt, EDT_PARAM_UNK, 0);
    ocrGuid_t edtGuid;
    u64 edtParamv[4] = {111,222,333,444};
    ocrEdtCreate(&edtGuid, remoteEdtTemplateGuid, 4, (u64*) &edtParamv, 0, NULL_GUID,
        EDT_PROP_NONE, edtAffinity, NULL);

    return NULL_GUID;
}
