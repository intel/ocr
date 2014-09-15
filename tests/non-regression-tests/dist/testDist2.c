/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - create a local event and a remote EDT, satisfy, then add-dep.
 */

ocrGuid_t remoteEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("[remote] RemoteEdt: executing\n");
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

    ocrGuid_t remoteEdtTemplateGuid;
    ocrEdtTemplateCreate(&remoteEdtTemplateGuid, remoteEdt, 0, 1);
    PRINTF("[local] mainEdt: create remote EDT\n");
    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, remoteEdtTemplateGuid, EDT_PARAM_DEF, NULL, EDT_PARAM_DEF, NULL_GUID,
        EDT_PROP_NONE, edtAffinity, NULL);
    ocrGuid_t eventGuid;
    PRINTF("[local] mainEdt: create local event\n");
    ocrEventCreate(&eventGuid, OCR_EVENT_STICKY_T, true);
    PRINTF("[local] mainEdt: satisfy local Event\n");
    ocrEventSatisfy(eventGuid, NULL_GUID);
    PRINTF("[local] mainEdt: add-dependence local Event, remote EDT\n");
    ocrAddDependence(eventGuid, edtGuid, 0, DB_MODE_RO);
    return NULL_GUID;
}
