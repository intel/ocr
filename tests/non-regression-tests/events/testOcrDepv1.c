/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */





#include "ocr.h"

/**
 * DESC: Create an EDT and directly give its depv
 */

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int* res = (int*)depv[0].ptr;
    PRINTF("In the taskForEdt with value %d\n", (*res));
    ASSERT(*res == 42);
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Creates a data block
    int *k;
    ocrGuid_t dbGuid;
    ocrDbCreate(&dbGuid,(void **) &k,
                sizeof(int), /*flags=*/0,
                /*location=*/NULL_GUID,
                NO_ALLOC);
    *k = 42;

    // Creates the EDT
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid,
                 EDT_PARAM_DEF, /*paramv=*/NULL,
                 EDT_PARAM_DEF, /*depv=*/&dbGuid,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    return NULL_GUID;
}
