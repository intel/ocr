#include "ocr.h"

// ex4: Setting up a pipeline, chaining EDTs upfront using output-events

ocrGuid_t stepA_edt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 * data = depv[0].ptr;
    ocrGuid_t dataGuid = depv[0].guid;
    u64 arraySize = paramv[0];
    // Do step A processing: initialize the array
    u64 i = 0;
    while(i < arraySize) {
        data[i] = 0;
        i++;
    }
    // The returned guid is channeled throught the EDT output-event if any
    return dataGuid;
}

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

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 arraySize = 100;
    //Create argument vector, first element is array size, second index to print
    u64 nparamv[1];
    nparamv[0] = arraySize;

    ocrGuid_t stepAEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepAEdtTemplateGuid, stepA_edt, 1 /*paramc*/, 1 /*depc*/);

    ocrGuid_t stepBEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepBEdtTemplateGuid, stepB_edt, 1 /*paramc*/, 1 /*depc*/);

    ocrGuid_t outputAGuid;
    //TODO Create stepA EDT with an output-event and provide a data-block
    // dependence later
    ocrGuid_t stepAGuid;
    ocrEdtCreate(&stepAGuid, stepAEdtTemplateGuid, EDT_PARAM_DEF, nparamv,
                 EDT_PARAM_DEF, NULL, /*prop=*/0, NULL_GUID, &outputAGuid);
    //END-TODO

    //TODO Create stepB depending on 'outputAGuid'
    ocrGuid_t stepBGuid;
    ocrEdtCreate(&stepBGuid, stepBEdtTemplateGuid, EDT_PARAM_DEF, nparamv,
                 EDT_PARAM_DEF, &outputAGuid, /*prop=*/0, NULL_GUID, NULL);
    //END-TODO

    // Setup datablock
    u64 * dataArray;
    ocrGuid_t dataGuid;
    ocrDbCreate(&dataGuid, (void **) &dataArray, sizeof(u64)*arraySize,
                /*flags=*/0, /*loc=*/NULL_GUID, NO_ALLOC);

    ocrAddDependence(dataGuid, stepAGuid, 0, DB_MODE_EW);
    // All dependences of stepA are provided

    return NULL_GUID;
}
