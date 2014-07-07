#include "ocr.h"

// ex1: Create an EDT

ocrGuid_t appEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // Use ocr-std.h for platform independence
    PRINTF("Hello from EDT\n");
    ocrShutdown(); // This is the last EDT to execute
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //TODO Create an EDT template for 'appEdt', no argument, no dependence
    ocrGuid_t appEdtTemplateGuid;
    ocrEdtTemplateCreate(&appEdtTemplateGuid, appEdt, 0 /*paramc*/, 0 /*depc*/);
    //END-TODO

    //TODO Create an EDT out of the EDT template
    ocrGuid_t appEdtGuid;
    ocrEdtCreate(&appEdtGuid, appEdtTemplateGuid, EDT_PARAM_DEF, NULL, EDT_PARAM_DEF, NULL,
                /*prop=*/EDT_PROP_NONE, NULL_GUID, NULL);
    //END-TODO
    return NULL_GUID;
}
