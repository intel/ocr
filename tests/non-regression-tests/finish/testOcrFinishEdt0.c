/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */





#include "ocr.h"

#define FLAGS 0xdead

/**
 * DESC: Simple finish-edt
 */

ocrGuid_t endEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t endEdtGuid;
    ocrGuid_t endEdtTemplateGuid;
    ocrEdtTemplateCreate(&endEdtTemplateGuid, endEdt, 0 /*paramc*/, 0 /*depc*/);
    ocrEdtCreate(&endEdtGuid, endEdtTemplateGuid, EDT_PARAM_DEF, /*paramv=*/NULL, EDT_PARAM_DEF, /*depv=*/NULL,
                 /*properties=*/ EDT_PROP_FINISH, NULL_GUID, /*outEvent=*/NULL);
    return NULL_GUID;
}
