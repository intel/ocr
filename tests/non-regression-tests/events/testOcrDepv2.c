/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

/**
 * DESC: Create and EDT and directly give its depv (which are NULL_GUIDS)
 */

ocrGuid_t taskForEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    assert(depv[0].guid == NULL_GUID);
    // This is the last EDT to execute, terminate
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Creates a data block

    ocrGuid_t ndepv[1];
    ndepv[0] = NULL_GUID;
    // Creates the EDT
    ocrGuid_t edtGuid;
    ocrGuid_t taskForEdtTemplateGuid;
    ocrEdtTemplateCreate(&taskForEdtTemplateGuid, taskForEdt, 0 /*paramc*/, 1 /*depc*/);
    ocrEdtCreate(&edtGuid, taskForEdtTemplateGuid,
                 EDT_PARAM_DEF, /*paramv=*/NULL,
                 EDT_PARAM_DEF, /*depv=*/ndepv,
                 /*properties=*/0, NULL_GUID, /*outEvent=*/NULL);

    return NULL_GUID;
}
