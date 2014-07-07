#include "ocr.h"

// ex5: Setting up a pipeline and use finish-edt to trigger the shutdown

ocrGuid_t stepB_edt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
   u64 arraySize = paramv[0];
    u64 * data = depv[0].ptr;
    // Do step B processing
    u64 i = 0;
    while(i < arraySize) {
        data[i]+=1;
        i++;
    }
    return NULL_GUID;
}

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

ocrGuid_t terminateEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown(); // This is the last EDT to execute, terminate
    return NULL_GUID;
}

// This is essentially the mainEdt of ex4
ocrGuid_t pipelineEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 arraySize = 100;
    //Create argument vector, first element is array size, second index to print
    u64 nparamv[1];
    nparamv[0] = arraySize;

    ocrGuid_t stepAEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepAEdtTemplateGuid, stepA_edt, 1 /*paramc*/, 1 /*depc*/);

    ocrGuid_t stepBEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepBEdtTemplateGuid, stepB_edt, 1 /*paramc*/, 1 /*depc*/);

    ocrGuid_t outputAGuid;
    //Create stepA EDT with an output-event and provide dependences later
    ocrGuid_t stepAGuid;
    ocrEdtCreate(&stepAGuid, stepAEdtTemplateGuid, EDT_PARAM_DEF, nparamv,
                 EDT_PARAM_DEF, NULL, /*prop=*/0, NULL_GUID, &outputAGuid);

    //Create stepB depending on 'outputAGuid'
    ocrGuid_t stepBGuid;
    ocrEdtCreate(&stepBGuid, stepBEdtTemplateGuid, EDT_PARAM_DEF, nparamv, EDT_PARAM_DEF, &outputAGuid,
                /*prop=*/0, NULL_GUID, NULL);

    // Setup datablock
    u64 * dataArray;
    ocrGuid_t dataGuid;
    ocrDbCreate(&dataGuid, (void **) &dataArray, sizeof(u64)*100, /*flags=*/0,
                /*loc=*/NULL_GUID, NO_ALLOC);

    //Setup datablock dependence to trigger stepA
    ocrAddDependence(dataGuid, stepAGuid, 0, DB_MODE_EW);
    // All dependences of stepA are provided

    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {

    ocrGuid_t pipeTemplateGuid;
    ocrEdtTemplateCreate(&pipeTemplateGuid, pipelineEdt, 0 /*paramc*/, 0 /*depc*/);

    ocrGuid_t outputEventGuid;
    //TODO Create pipeline EDT as a finish-EDT
    ocrGuid_t pipeGuid;
    ocrEdtCreate(&pipeGuid, pipeTemplateGuid, EDT_PARAM_DEF, NULL, EDT_PARAM_DEF, NULL,
                /*prop=*/EDT_PROP_FINISH, NULL_GUID, &outputEventGuid);
    //END-TODO

    // In this example we chain the output-event of the pipeline EDT
    // with the terminate EDT. Only when the pipeline EDT and all its
    // transitively created children EDT have been executed, the
    // output-event is satisfied
    ocrGuid_t terminateTemplateGuid;
    ocrEdtTemplateCreate(&terminateTemplateGuid, terminateEdt, 0 /*paramc*/, 1 /*depc*/);

    ocrGuid_t terminateGuid;
    //TODO Create terminate EDT depending on the output-event of the pipeline EDT
    ocrEdtCreate(&terminateGuid, terminateTemplateGuid, EDT_PARAM_DEF, 0,
                 EDT_PARAM_DEF, &outputEventGuid, /*prop=*/0, NULL_GUID, NULL);
    //END-TODO
    return NULL_GUID;
}
