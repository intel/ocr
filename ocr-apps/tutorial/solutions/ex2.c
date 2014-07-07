#include "ocr.h"

// ex2: Passing arguments and setting up dependence

ocrGuid_t stepA_edt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //TODO Retrieve the data-block pointer
    u64 * data = depv[0].ptr;
    //END-TODO
    PRINTF("Array size: %lu, array[%lu]: %lu\n", paramv[0], paramv[1], data[paramv[1]]);
    ocrShutdown(); // This is the last EDT to execute
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //TODO Create a template for 'stepA_edt' taking two parameters and an unknow number of dependences
    ocrGuid_t stepAEdtTemplateGuid;
    ocrEdtTemplateCreate(&stepAEdtTemplateGuid, stepA_edt, 2 /*paramc*/,
                         EDT_PARAM_UNK /*depc*/);
    //END-TODO

    u64 arraySize = 100;
    u64 index = 2;
    //TODO Create argument vector, first element is array size, second index to print
    u64 nparamv[2];
    nparamv[0] = arraySize;
    nparamv[1] = index;
    //END-TODO
    u64 * dataArray;
    ocrGuid_t dataGuid;
    //TODO Create datablock to hold a block of 'array size' u64 elements
    ocrDbCreate(&dataGuid, (void **) &dataArray, sizeof(u64)*arraySize,
                /*flags=*/0, /*loc=*/NULL_GUID, NO_ALLOC);
    //END-TODO

    // Set element at 'index'
    dataArray[index] = 10;

    //TODO Create an EDT passing the parameter vector and the data-block as dependence
    //This is equivalent to directly satisffying the first dependence slot of the EDT
    ocrGuid_t stepAEdtGuid;
    ocrEdtCreate(&stepAEdtGuid, stepAEdtTemplateGuid, EDT_PARAM_DEF, nparamv, 1, &dataGuid,
                /*prop=*/EDT_PROP_NONE, NULL_GUID, NULL);
    //END-TODO

    return NULL_GUID;
}
