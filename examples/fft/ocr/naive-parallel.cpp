/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// OCR implementation of the Cooley-Tukey algorithm with simple recursion.
// Each StartEDT delegates its work to three children - two StartEDTs on half the data each,
// and a EndEDT to multiply all coefficients by the twiddle factors.
//

#define _USE_MATH_DEFINES

#include <stack>

#include "options.h"
#include "verify.h"

#include "ocr.h"

// Performs one entire iteration of FFT.
// These are meant to be chained serially for timing and testing.
ocrGuid_t fftIterationEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrGuid_t startTempGuid = paramv[0];
    bool verbose = paramv[6];
    if(verbose) {
        PRINTF("Creating iteration child\n");
    }

    ocrGuid_t dependencies[1] = { depv[0].guid };

    ocrGuid_t edtGuid;
    ocrEdtCreate(&edtGuid, startTempGuid, EDT_PARAM_DEF, paramv, EDT_PARAM_DEF,
                 dependencies, EDT_PROP_FINISH, NULL_GUID, NULL_GUID);

    return NULL_GUID;
}

ocrGuid_t fftStartEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    ocrGuid_t startGuid = paramv[0];
    ocrGuid_t endGuid = paramv[1];
    float *data = (float*)depv[0].ptr;
    ocrGuid_t dataGuid = depv[0].guid;
    u64 N = paramv[2];
    u64 step = paramv[3];
    u64 offset = paramv[4];
    u64 x_in_offset = paramv[5];
    bool verbose = paramv[6];
    bool printResults = paramv[7];
    float *x_in = (float*)data;
    float *X_real = (float*)(data+offset + N*step);
    float *X_imag = (float*)(data+offset + 2*N*step);

    if(N == 1) {
        X_real[0] = x_in[x_in_offset];
        X_imag[0] = 0;
    } else {
        // DFT even side
        u64 childParamv[8] = { startGuid, endGuid, N/2, 2 * step, 0 + offset,
                               x_in_offset, verbose, printResults };
        u64 childParamv2[8] = { startGuid, endGuid, N/2, 2 * step, N/2 + offset,
                                x_in_offset + step, verbose, printResults };

        if(verbose) {
            PRINTF("Creating children of size %d\n",N/2);
        }
        ocrGuid_t edtGuid, edtGuid2, endEdtGuid, finishEventGuid, finishEventGuid2;

        ocrEdtCreate(&edtGuid, startGuid, EDT_PARAM_DEF, childParamv,
                     EDT_PARAM_DEF, NULL_GUID, EDT_PROP_FINISH,
                     NULL_GUID, &finishEventGuid);
        ocrEdtCreate(&edtGuid2, startGuid, EDT_PARAM_DEF,
                     childParamv2, EDT_PARAM_DEF, NULL_GUID,
                     EDT_PROP_FINISH, NULL_GUID, &finishEventGuid2);
        if(verbose) {
            PRINTF("finishEventGuid after create: %lu\n",finishEventGuid);
        }

        ocrGuid_t endDependencies[3] = { dataGuid, finishEventGuid, finishEventGuid2 };
        // Do calculations after having divided and conquered
        ocrEdtCreate(&endEdtGuid, endGuid, EDT_PARAM_DEF, paramv,
                     EDT_PARAM_DEF, endDependencies, EDT_PROP_FINISH, NULL_GUID,
                     NULL);

        ocrAddDependence(dataGuid, edtGuid, 0, DB_MODE_ITW);
        ocrAddDependence(dataGuid, edtGuid2, 0, DB_MODE_ITW);
    }


    if(verbose) {
        PRINTF("Task with size %d completed\n",N);
    }
    return NULL_GUID;
}

ocrGuid_t fftEndEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    ocrGuid_t startGuid = paramv[0];
    ocrGuid_t endGuid = paramv[1];
    float *data = (float*)depv[0].ptr;
    ocrGuid_t dataGuid = depv[0].guid;
    u64 N = paramv[2];
    u64 step = paramv[3];
    u64 offset = paramv[4];
    bool verbose = paramv[6];
    bool printResults = paramv[7];
    float *x_in = (float*)data+offset;
    float *X_real = (float*)(data+offset + N*step);
    float *X_imag = (float*)(data+offset + 2*N*step);
    if(verbose) {
        PRINTF("Reached end phase for step %d\n",step);
    }

    int k;
    for(k=0;k<N/2;k++) {
        float t_real = X_real[k];
        float t_imag = X_imag[k];
        double twiddle_real;
        double twiddle_imag;
        twiddle_imag = sin(-2 * M_PI * k / N);
        twiddle_real = cos(-2 * M_PI * k / N);
        float xr = X_real[k+N/2];
        float xi = X_imag[k+N/2];

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
    return NULL_GUID;
}

ocrGuid_t finalPrintEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    ocrGuid_t startGuid = paramv[0];
    ocrGuid_t endGuid = paramv[1];
    float *data = (float*)depv[1].ptr;
    ocrGuid_t dataGuid = depv[1].guid;
    u64 N = paramv[2];
    u64 step = paramv[3];
    u64 offset = paramv[4];
    bool verbose = paramv[6];
    bool printResults = paramv[7];
    float *x_in = (float*)data+offset;
    float *X_real = (float*)(data+offset + N*step);
    float *X_imag = (float*)(data+offset + 2*N*step);

    if(verbose) {
        PRINTF("Final print EDT\n");
    }

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
    ocrGuid_t startTempGuid,iterationTempGuid,endTempGuid,printTempGuid;
    ocrEdtTemplateCreate(&startTempGuid, &fftStartEdt, 8, 1);
    ocrEdtTemplateCreate(&iterationTempGuid, &fftIterationEdt, 8, 2);
    ocrEdtTemplateCreate(&endTempGuid, &fftEndEdt, 8, 3);
    ocrEdtTemplateCreate(&printTempGuid, &finalPrintEdt, 8, 2);

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
    if(!parseOptions(argc,argv,&N,&verify,&iterations,&verbose,&printResults,NULL)) {
        printHelp(argv,false);
        ocrShutdown();
        return NULL_GUID;
    }
    if(verbose) {
        for(i=0;i<argc;i++) {
            PRINTF("argv[%d]: %s\n",i,argv[i]);
        }
        PRINTF("Running %d iterations\n",iterations);
    }

    // x_in, X_real, and X_imag in a contiguous block
    float *x;
    ocrGuid_t dataGuid;
    ocrDbCreate(&dataGuid, (void **) &x, sizeof(float) * N * 3, 0, NULL_GUID, NO_ALLOC);

    for(i=0;i<N;i++) {
        x[i] = 0;
    }
    x[1] = 1;




    // Create an EDT out of the EDT template
    ocrGuid_t edtGuid, printEdtGuid, edtEventGuid;

    std::stack<ocrGuid_t> edtStack;
    std::stack<ocrGuid_t> eventStack;
    edtEventGuid = NULL_GUID;
    ocrGuid_t edtPrevEventGuid = NULL_GUID;
    u64 edtParamv[8] = { startTempGuid, endTempGuid, N, 1 /* step size */,
                         0 /* offset */, 0 /* x_in_offset */, verbose, printResults};

    for(i=1;i<=iterations;i++) {
        ocrEdtCreate(&edtGuid, iterationTempGuid, EDT_PARAM_DEF, edtParamv,
                     EDT_PARAM_DEF, NULL_GUID, EDT_PROP_FINISH, NULL_GUID,
                     &edtEventGuid);
        edtStack.push(edtGuid);
        eventStack.push(edtEventGuid);
    }

    edtEventGuid = eventStack.top();
    if(verify) {
        edtEventGuid = setUpVerify(dataGuid, NULL_GUID, NULL_GUID, N, edtEventGuid);
    }

    ocrGuid_t finishDependencies[2] = { edtEventGuid, dataGuid };
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
        ocrAddDependence(dataGuid, edtGuid, 0, DB_MODE_ITW);
        ocrAddDependence(edtEventGuid, edtGuid, 1, DB_MODE_RO);
        edtStack.pop();
        eventStack.pop();
    }

    return NULL_GUID;
}
