
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */




#include "ocr.h"
#include "extensions/ocr-affinity.h"

/**
 * DESC: OCR-DIST - Writes to a DB/SA and double check changes are not seen remotely.
 * This is illegal, and the runtime doesn't check for it. However if the implementation
 * is correct the changes should only be seen on the local node.
 */

#define TYPE_ELEM_DB int
#define NB_ELEM_DB 20

ocrGuid_t checkerEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbGuid = (ocrGuid_t) depv[0].guid;
    PRINTF("[remote] checkerEdt: executing, depends on remote DB guid 0x%lx \n", dbGuid);
    TYPE_ELEM_DB v = 1;
    int i = 0;
    TYPE_ELEM_DB * data = (TYPE_ELEM_DB *) depv[0].ptr;
    while (i < NB_ELEM_DB) {
        ASSERT (data[i] == v++);
        i++;
    }
    PRINTF("[remote] checkerEdt: DB/SA copy checked\n");
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t saViolationEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t dbGuid = (ocrGuid_t) depv[0].guid;
    PRINTF("[remote] saViolationEdt: executing, depends on remote DB guid 0x%lx \n", dbGuid);
    TYPE_ELEM_DB v = 1;
    int i = 0;
    TYPE_ELEM_DB * data = (TYPE_ELEM_DB *) depv[0].ptr;
    while (i < NB_ELEM_DB) {
        ASSERT (data[i] == v++);
        i++;
    }
    PRINTF("[remote] saViolationEdt: illegal local writes to DB/SA\n");

    // Being a bad boy and trying to modify the DB/SA content.
    // There's nothing the runtime can do to prevent spoiling
    // this node copy (beside making an extra preventing copy).
    // However the runtime guarantees this won't be written-back
    // to the DB/SA located on its home node
    while (i < NB_ELEM_DB) {
        data[i] *= 2;
        i++;
    }
    ocrDbRelease(dbGuid);

    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t edtAffinity = affinities[0]; //TODO this implies we know current PD is '0'

    ocrGuid_t checkerTemplateGuid;
    ocrEdtTemplateCreate(&checkerTemplateGuid, checkerEdt, 0, 1);

    PRINTF("[remote] saViolationEdt: create checker EDT at DB/SA home node\n");
    ocrGuid_t checkerEdtGuid;
    ocrEdtCreate(&checkerEdtGuid, checkerTemplateGuid, 0, NULL, 1, &dbGuid,
                 EDT_PROP_NONE, edtAffinity, NULL);
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 affinityCount;
    ocrAffinityCount(AFFINITY_PD, &affinityCount);
    ASSERT(affinityCount >= 1);
    ocrGuid_t affinities[affinityCount];
    ocrAffinityGet(AFFINITY_PD, &affinityCount, affinities);
    ocrGuid_t edtAffinity = affinities[affinityCount-1]; //TODO this implies we know current PD is '0'

    // Create a DB
    void * dbPtr;
    ocrGuid_t dbGuid;
    u64 nbElem = NB_ELEM_DB;
    ocrDbCreate(&dbGuid, &dbPtr, sizeof(TYPE_ELEM_DB) * NB_ELEM_DB, 0, NULL_GUID, NO_ALLOC);
    int v = 1;
    int i = 0;
    int * data = (int *) dbPtr;
    while (i < nbElem) {
        data[i] = v++;
        i++;
    }
    ocrDbRelease(dbGuid);
    PRINTF("[local] mainEdt: local DB guid is 0x%lx, dbPtr=%p\n",dbGuid, dbPtr);

    // create local edt that depends on the remote edt, the db is automatically cloned
    ocrGuid_t saViolationTemplateGuid;
    ocrEdtTemplateCreate(&saViolationTemplateGuid, saViolationEdt, 0, 1);

    ocrGuid_t saViolationEdtGuid;
    ocrEdtCreate(&saViolationEdtGuid, saViolationTemplateGuid, 0, NULL, 1, &dbGuid,
                 EDT_PROP_NONE, edtAffinity, NULL);


    return NULL_GUID;
}
