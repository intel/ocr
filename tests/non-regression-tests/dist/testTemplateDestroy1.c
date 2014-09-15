/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create a remote EDT and destroy it inside the EDT
 */

ocrGuid_t shutdownEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("[remote] RemoteEdt: executing\n");
    ocrGuid_t edtTemplateGuid = (ocrGuid_t) paramv[0];
    ocrEdtTemplateDestroy(edtTemplateGuid);

    // shutdown
    ocrGuid_t templ;
    ocrEdtTemplateCreate(&templ, shutdownEdt, 0, 0);
    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, templ, 0, NULL, 0, NULL_GUID,
        EDT_PROP_NONE, NULL_GUID, NULL);

    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t edtAffinity = affinities[affinityCount-1];

    // create local edt that depends on the remote edt, the db is automatically cloned
    ocrGuid_t edtTemplateGuid;
    ocrEdtTemplateCreate(&edtTemplateGuid, remoteEdt, 1, 0);

    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, edtTemplateGuid, 1, (u64*) &edtTemplateGuid, 0, NULL_GUID,
        EDT_PROP_NONE, edtAffinity, NULL);

    return NULL_GUID;
}
