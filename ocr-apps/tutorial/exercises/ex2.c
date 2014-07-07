#include "ocr.h"

// ex2: Passing arguments and setting up dependence

ocrGuid_t stepA_edt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //TODO Retrieve the data-block pointer
    //END-TODO
    PRINTF("Array size: %lu, array[%lu]: %lu\n", paramv[0], paramv[1], data[paramv[1]]);
    ocrShutdown(); // This is the last EDT to execute
    return NULL_GUID;
}

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    //TODO Create a template for 'stepA_edt' taking two parameters and an unknown
    //number of dependences
    //END-TODO

    u64 arraySize = 100;
    u64 index = 2;
    //TODO Create argument vector, first element is array size, second index to print
    //END-TODO
    u64 * dataArray;
    ocrGuid_t dataGuid;
    //TODO Create datablock to hold a block of 'array size' u64 elements
    //END-TODO

    // Set element at 'index'
    dataArray[index] = 10;

    //TODO Create an EDT passing the parameter vector and the data-block as dependence
    //This is equivalent to directly satisffying the first dependence slot of the EDT
    //END-TODO

    return NULL_GUID;
}
