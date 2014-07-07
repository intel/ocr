/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// Parallel version of Cooley-Tukey FFT algorithm.
// The problem is divided into smaller chunks of half size recursively,
// until the base size serialBlockSize is reached. These chunks are
// processed by EDTs, which write the result for their parents to finish processing.
//

#define _USE_MATH_DEFINES

#include <stack>

#include "options.h"
#include "verify.h"
#include "timer.h"

#define SERIAL_BLOCK_SIZE_DEFAULT (1024*16)

#include <ocr.h>

// Performs one entire iteration of FFT.
// These are meant to be chained serially for timing and testing.
ocrGuid_t fftIterationEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t startTempGuid = paramv[0];
    ocrGuid_t endTempGuid = paramv[1];
    ocrGuid_t endSlaveTempGuid = paramv[2];
    u64 N = paramv[3];
    bool verbose = paramv[4];
    u64 serialBlockSize = paramv[5];
    float *x_in = (float*)depv[0].ptr;
    float *X_real = (float*)depv[1].ptr;
    float *X_imag = (float*)depv[2].ptr;
    int i;

    if(verbose) {
        PRINTF("Hello from EDT %lu\n",paramv[6]);
    }

    u64 edtParamv[9] = { startTempGuid, endTempGuid, endSlaveTempGuid,
                         N, 1 /* step size */, 0 /* offset */,
                         0 /* x_in_offset */, verbose, serialBlockSize };
    ocrGuid_t dependencies[3] = { depv[0].guid, depv[1].guid, depv[2].guid };

    if(verbose) {
        PRINTF("Creating iteration child\n");
    }
    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, startTempGuid, EDT_PARAM_DEF, edtParamv, EDT_PARAM_DEF,
                 dependencies, EDT_PROP_FINISH, NULL_GUID, NULL_GUID);

    return NULL_GUID;
}

// First part of the Cooley-Tukey algorithm.
// The work is recursively split in half until N = serialBlockSize.
ocrGuid_t fftStartEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    ocrGuid_t startGuid = paramv[0];
    ocrGuid_t endGuid = paramv[1];
    ocrGuid_t endSlaveGuid = paramv[2];
    float *data_in = (float*)depv[0].ptr;
    float *data_real = (float*)depv[1].ptr;
    float *data_imag = (float*)depv[2].ptr;
    ocrGuid_t dataInGuid = depv[0].guid;
    ocrGuid_t dataRealGuid = depv[1].guid;
    ocrGuid_t dataImagGuid = depv[2].guid;
    u64 N = paramv[3];
    u64 step = paramv[4];
    u64 offset = paramv[5];
    u64 x_in_offset = paramv[6];
    bool verbose = paramv[7];
    u64 serialBlockSize = paramv[8];
    float *x_in = (float*)data_in;
    float *X_real = (float*)(data_real+offset);
    float *X_imag = (float*)(data_imag+offset);
    if(verbose) {
        PRINTF("Step %d offset: %d N*step: %d\n", step, offset, N*step);
    }

    if(N <= serialBlockSize) {
        ditfft2(X_real, X_imag, x_in+x_in_offset, N, step);
    } else {
        // DFT even side
        u64 childParamv[9] = { startGuid, endGuid, endSlaveGuid, N/2, 2 * step,
                               0 + offset, x_in_offset, verbose, serialBlockSize };
        u64 childParamv2[9] = { startGuid, endGuid, endSlaveGuid, N/2, 2 * step,
                                N/2 + offset, x_in_offset + step, verbose, serialBlockSize };

        if(verbose) {
            PRINTF("Creating children of size %d\n",N/2);
        }
        ocrGuid_t edtGuid, edtGuid2, endEdtGuid, finishEventGuid,
                finishEventGuid2;

        // Divide-and-conquer child EDTs will not need finish event guids.
        // This dependency list is the same as endDependencies below minus
        // the finish event guids.
        ocrGuid_t childDependencies[3] = { dataInGuid, dataRealGuid, dataImagGuid };

        ocrEdtCreate(&edtGuid, startGuid, EDT_PARAM_DEF, childParamv,
                     EDT_PARAM_DEF, NULL_GUID, EDT_PROP_FINISH, NULL_GUID,
                     &finishEventGuid);
        ocrEdtCreate(&edtGuid2, startGuid, EDT_PARAM_DEF, childParamv2,
                     EDT_PARAM_DEF, NULL_GUID, EDT_PROP_FINISH, NULL_GUID,
                     &finishEventGuid2);
        if(verbose) {
            PRINTF("Created child EDTs\n");
            PRINTF("finishEventGuid after create: %lu\n",finishEventGuid);
        }

        ocrGuid_t endDependencies[5] = { dataInGuid, dataRealGuid, dataImagGuid,
                                         finishEventGuid, finishEventGuid2 };

        // Do calculations after having divided and conquered
        ocrEdtCreate(&endEdtGuid, endGuid, EDT_PARAM_DEF, paramv, EDT_PARAM_DEF,
                     endDependencies, EDT_PROP_FINISH, NULL_GUID, NULL);
        if(verbose) {
            PRINTF("Created end EDT\n");
        }

        ocrAddDependence(dataInGuid, edtGuid, 0, DB_MODE_RO);
        ocrAddDependence(dataRealGuid, edtGuid, 1, DB_MODE_ITW);
        ocrAddDependence(dataImagGuid, edtGuid, 2, DB_MODE_ITW);
        ocrAddDependence(dataInGuid, edtGuid2, 0, DB_MODE_RO);
        ocrAddDependence(dataRealGuid, edtGuid2, 1, DB_MODE_ITW);
        ocrAddDependence(dataImagGuid, edtGuid2, 2, DB_MODE_ITW);
    }

    if(verbose) {
        PRINTF("Task with size %d completed\n",N);
    }
    return NULL_GUID;
}

// Handles the second part of the Cooley-Tukey algorithm, performing calculations on
// The entire set of coefficients. The work is again split to be computed in parallel
// by a number of slaves.
ocrGuid_t fftEndEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    ocrGuid_t startGuid = paramv[0];
    ocrGuid_t endGuid = paramv[1];
    ocrGuid_t endSlaveGuid = paramv[2];
    float *data_in = (float*)depv[0].ptr;
    float *data_real = (float*)depv[1].ptr;
    float *data_imag = (float*)depv[2].ptr;
    ocrGuid_t dataGuids[3] = { depv[0].guid, depv[1].guid, depv[2].guid };
    u64 N = paramv[3];
    u64 step = paramv[4];
    u64 offset = paramv[5];
    bool verbose = paramv[7];
    u64 serialBlockSize = paramv[8];
    float *x_in = (float*)data_in+offset;
    float *X_real = (float*)(data_real+offset);
    float *X_imag = (float*)(data_imag+offset);
    if(verbose) {
        PRINTF("Reached end phase for step %d\n",step);
        PRINTF("paramc: %d\n",paramc);
    }

    if(N/2 > serialBlockSize) {
        ocrGuid_t slaveGuids[(N/2)/serialBlockSize];
        u64 slaveParamv[5 * (N/2)/serialBlockSize];

        if(verbose) {
            PRINTF("Creating %d slaves for N=%d\n",(N/2)/serialBlockSize,N);
        }

        for(i=0;i<(N/2)/serialBlockSize;i++) {
            slaveParamv[i*5] = N;
            slaveParamv[i*5+1] = step;
            slaveParamv[i*5+2] = offset;
            slaveParamv[i*5+3] = i*serialBlockSize;
            slaveParamv[i*5+4] = (i+1)*serialBlockSize;

            ocrEdtCreate(slaveGuids+i, endSlaveGuid, EDT_PARAM_DEF,
                         slaveParamv+i*5, EDT_PARAM_DEF, dataGuids,
                         EDT_PROP_NONE, NULL_GUID, NULL);
        }
    } else {
        ocrGuid_t slaveGuids[1];
        u64 slaveParamv[5];

        slaveParamv[0] = N;
        slaveParamv[1] = step;
        slaveParamv[2] = offset;
        slaveParamv[3] = 0;
        slaveParamv[4] = N/2;

        ocrEdtCreate(slaveGuids, endSlaveGuid, EDT_PARAM_DEF, slaveParamv,
                     EDT_PARAM_DEF, dataGuids, EDT_PROP_NONE, NULL_GUID, NULL);
    }
    return NULL_GUID;
}

ocrGuid_t fftEndSlaveEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    float *data_in = (float*)depv[0].ptr;
    float *data_real = (float*)depv[1].ptr;
    float *data_imag = (float*)depv[2].ptr;
    u64 N = paramv[0];
    u64 step = paramv[1];
    u64 offset = paramv[2];
    float *x_in = (float*)data_in+offset;
    float *X_real = (float*)(data_real+offset);
    float *X_imag = (float*)(data_imag+offset);
    u64 kStart = paramv[3];
    u64 kEnd = paramv[4];

    int k;
    for(k=kStart;k<kEnd;k++) {
        float t_real = X_real[k];
        float t_imag = X_imag[k];
        float xr = X_real[k+N/2];
        float xi = X_imag[k+N/2];
        double twiddle_real;
        double twiddle_imag;
        twiddle_imag = sin(-2 * M_PI * k / N);
        twiddle_real = cos(-2 * M_PI * k / N);

        // (a+bi)(c+di) = (ac - bd) + (bc + ad)i
        X_real[k] = t_real +
            (twiddle_real*xr - twiddle_imag*xi);
        X_imag[k] = t_imag +
            (twiddle_imag*xr + twiddle_real*xi);
        X_real[k+N/2] = t_real -
            (twiddle_real*xr - twiddle_imag*xi);
        X_imag[k+N/2] = t_imag -
            (twiddle_imag*xr + twiddle_real*xi);
    }
//    sleep(1);

    return NULL_GUID;
}

// Prints the final result of the computation. Called as the last EDT.
ocrGuid_t finalPrintEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    u64 N = paramv[0];
    bool verbose = paramv[1];
    bool printResults = paramv[2];
    float *data_in = (float*)depv[1].ptr;
    float *data_real = (float*)depv[2].ptr;
    float *data_imag = (float*)depv[3].ptr;
    float *x_in = (float*)data_in;
    float *X_real = (float*)(data_real);
    float *X_imag = (float*)(data_imag);
    double *startTime = (double*)(depv[4].ptr);

    if(verbose) {
        PRINTF("Final print EDT\n");
    }

    double endTime = mysecond();
    PRINTF("%f\n",endTime-*startTime);

    if(printResults) {
        PRINTF("Starting values:\n");
        for(i=0;i<N;i++) {
            PRINTF("%d [ %f ]\n",i,x_in[i]);
        }
        PRINTF("\n");

        PRINTF("Final result:\n");
        for(i=0;i<N;i++) {
            PRINTF("%d [%f + %fi]\n",i,X_real[i],X_imag[i]);
        }
    }
    ocrShutdown();
}

extern "C" ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 argc = getArgc(depv[0].ptr);
    int i;
    char *argv[argc];
    for(i=0;i<argc;i++) {
        argv[i] = getArgv(depv[0].ptr,i);
    }

    u64 N;
    u64 iterations;
    bool verify;
    bool verbose;
    bool printResults;
    u64 serialBlockSize = SERIAL_BLOCK_SIZE_DEFAULT;
    if(!parseOptions(argc, argv, &N, &verify, &iterations, &verbose, &printResults,
                     &serialBlockSize)) {
        printHelp(argv,true);
        ocrShutdown();
        return NULL_GUID;
    }
    if(verbose) {
        for(i=0;i<argc;i++) {
            PRINTF("argv[%d]: %s\n",i,argv[i]);
        }
    }
    if(iterations > 1 && verbose) {
        PRINTF("Running %d iterations\n",iterations);
    }

    ocrGuid_t startTempGuid,endTempGuid,printTempGuid,endSlaveTempGuid,iterationTempGuid;
    ocrEdtTemplateCreate(&iterationTempGuid, &fftIterationEdt, 7, 4);
    ocrEdtTemplateCreate(&startTempGuid, &fftStartEdt, 9, 3);
    ocrEdtTemplateCreate(&endTempGuid, &fftEndEdt, 9, 5);
    ocrEdtTemplateCreate(&endSlaveTempGuid, &fftEndSlaveEdt, 5, 3);
    ocrEdtTemplateCreate(&printTempGuid, &finalPrintEdt, 3, 5);

    float *x_in;
    // Output for the FFT
    float *X_real;
    float *X_imag;
    ocrGuid_t dataInGuid,dataRealGuid,dataImagGuid,timeDataGuid;

    // TODO: OCR cannot handle large datablocks
    ocrDbCreate(&dataInGuid, (void **) &x_in, sizeof(float) * N, 0, NULL_GUID, NO_ALLOC);
    ocrDbCreate(&dataRealGuid, (void **) &X_real, sizeof(float) * N, 0, NULL_GUID, NO_ALLOC);
    ocrDbCreate(&dataImagGuid, (void **) &X_imag, sizeof(float) * N, 0, NULL_GUID, NO_ALLOC);
    if(verbose) {
        PRINTF("3 Datablocks of size %lu (N=%lu) created\n",sizeof(float)*N,N);
    }

    for(i=0;i<N;i++) {
        x_in[i] = 0;
        X_real[i] = 0;
        X_imag[i] = 0;
    }
    x_in[1] = 1;
    //x_in[3] = -1;
    //x_in[5] = 1;
    //x_in[7] = -1;


    // Create an EDT out of the EDT template
    ocrGuid_t edtGuid, edtPrevGuid, printEdtGuid, edtEventGuid, edtPrevEventGuid;
    //ocrEdtCreate(&edtGuid, startTempGuid, EDT_PARAM_DEF, edtParamv,
    //             EDT_PARAM_DEF, NULL_GUID, EDT_PROP_FINISH, NULL_GUID, &edtEventGuid);

    std::stack<ocrGuid_t> edtStack;
    std::stack<ocrGuid_t> eventStack;
    edtEventGuid = NULL_GUID;
    edtPrevEventGuid = NULL_GUID;

    for(i=1;i<=iterations;i++) {
        u64 edtParamv[7] = { startTempGuid, endTempGuid, endSlaveTempGuid, N,
                             verbose, serialBlockSize, i };
        ocrEdtCreate(&edtGuid, iterationTempGuid, EDT_PARAM_DEF, edtParamv,
                     EDT_PARAM_DEF, NULL_GUID, EDT_PROP_FINISH, NULL_GUID, &edtEventGuid);
        edtStack.push(edtGuid);
        eventStack.push(edtEventGuid);
    }

    edtEventGuid = eventStack.top();
    if(verify) {
        edtEventGuid = setUpVerify(dataInGuid, dataRealGuid, dataImagGuid, N,
                                   edtEventGuid);
    }
    double *startTime;
    ocrDbCreate(&timeDataGuid, (void **) &startTime, sizeof(double), 0, NULL_GUID, NO_ALLOC);

    *startTime = mysecond();
    u64 edtParamv[3] = { N, verbose, printResults };
    // Create finish EDT, with dependence on last EDT

    ocrGuid_t finishDependencies[5] = { edtEventGuid, dataInGuid, dataRealGuid,
                                        dataImagGuid, timeDataGuid };
    ocrEdtCreate(&printEdtGuid, printTempGuid, EDT_PARAM_DEF, edtParamv,
                 EDT_PARAM_DEF, finishDependencies, EDT_PROP_NONE, NULL_GUID, NULL);
    eventStack.pop();

    while(!edtStack.empty()) {
        edtGuid = edtStack.top();
        if(!eventStack.empty()) {
            edtEventGuid = eventStack.top();
        } else {
            edtEventGuid = NULL_GUID;
        }
        ocrAddDependence(dataInGuid, edtGuid, 0, DB_MODE_RO);
        ocrAddDependence(dataRealGuid, edtGuid, 1, DB_MODE_ITW);
        ocrAddDependence(dataImagGuid, edtGuid, 2, DB_MODE_ITW);
        ocrAddDependence(edtEventGuid, edtGuid, 3, DB_MODE_RO);
        edtStack.pop();
        eventStack.pop();
    }

    return NULL_GUID;
}
