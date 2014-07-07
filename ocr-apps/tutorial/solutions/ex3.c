#include "ocr.h"

// ex3: EDTs spawning other EDTs

ocrGuid_t stepB_edt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 arraySize = paramv[0];
    u64 * data = depv[0].ptr;
    // Do step B processing
    u64 i = 0;
    while(i < arraySize) {
        data[i]+=1;
        i++;
    }
    ocrShutdown(); // This is the last EDT to execute
    return NULL_GUID;
}

ocrGuid_t stepA_edt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 * data = depv[0].ptr;
    //TODO retrieve data-block guid from depv
    ocrGuid_t dataGuid = depv[0].guid;
    //END-TODO
    u64 arraySize = paramv[0];

    // Do step A processing: initialize the array
    u64 i = 0;
    while(i < arraySize) {
        data[i] = 0;
        i++;
    }

    // Setup next step
    ocrGuid_t stepBEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepBEdtTemplateGuid, stepB_edt, 1 /*paramc*/, 1 /*depc*/);

    ocrGuid_t stepBEdtGuid;
    ocrEdtCreate(&stepBEdtGuid, stepBEdtTemplateGuid, EDT_PARAM_DEF, paramv,
                 EDT_PARAM_DEF, &dataGuid, /*prop=*/EDT_PROP_NONE, NULL_GUID, NULL);

    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {

    u64 arraySize = 100;
    //Create argument vector to hold array size
    u64 nparamv[1];
    nparamv[0] = arraySize;

    u64 * dataArray;
    ocrGuid_t dataGuid;
    //Create datablock to hold a block of 'array size' u64 elements
    ocrDbCreate(&dataGuid, (void **) &dataArray, sizeof(u64)*arraySize,
                /*flags=*/0, /*loc=*/NULL_GUID, NO_ALLOC);

    ocrGuid_t stepAEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepAEdtTemplateGuid, stepA_edt, 1 /*paramc*/, 1 /*depc*/);

    // Create the EDT not specifying the dependence vector at creation
    ocrGuid_t stepAEdtGuid;
    ocrEdtCreate(&stepAEdtGuid, stepAEdtTemplateGuid, EDT_PARAM_DEF, nparamv, 1, NULL,
                /*prop=*/EDT_PROP_NONE, NULL_GUID, NULL);

    ocrGuid_t triggerEventGuid;
    //TODO Setup event used to trigger stepA
    ocrEventCreate(&triggerEventGuid, OCR_EVENT_STICKY_T, true);
    //END-TODO

    //TODO Setup dependence between event and stepA's EDTs slot 0
    ocrAddDependence(triggerEventGuid, stepAEdtGuid, 0, DB_MODE_EW);
    //END-TODO

    //TODO Satisfy the event with the datablock
    ocrEventSatisfy(triggerEventGuid, dataGuid);
    //END-TODO

    return NULL_GUID;
}
