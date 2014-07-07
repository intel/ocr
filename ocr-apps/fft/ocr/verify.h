/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <ocr.h>

#include <cmath>
#include <limits>

#include "ditfft2.h"


bool areSame(double a, double b) {
    return std::fabs(a-b) < std::numeric_limits<double>::epsilon();
}

// Checks whether the 3 input datablocks (in, X_real and X_imag) match the results
// of a serial FFT.
//
// Params: N, number of elements
//
// Dependencies:    0) x_in
//            1) X_real
//            2) X_imag
//            3) user-defined event, allowing the EDT to be triggered on-demand
ocrGuid_t fftVerifyEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    int i;
    float *data_in;
    float *X_real_other;
    float *X_imag_other;
    // Create new data for duplicating X_real and X_imag's results
    float *X_real;
    float *X_imag;
    u64 N = paramv[0];
    PRINTF("Starting verification. depc:%d\n",depc);
    if(depc > 4) {
        data_in = (float*)depv[0].ptr;
        X_real_other = (float*)depv[1].ptr;
        X_imag_other = (float*)depv[2].ptr;
        X_real = (float*)depv[3].ptr;
        X_imag = (float*)depv[4].ptr;
    } else {
        data_in = (float*)depv[0].ptr;
        X_real_other = data_in+N;
        X_imag_other = X_real_other+N;
        X_real = (float*)depv[1].ptr;
        X_imag = (float*)depv[2].ptr;
    }
    bool intact = true;

    for(i=0;i<N;i++) {
        X_real[i] = 0;
        X_imag[i] = 0;
    }
    PRINTF("Finished initializing.\n");

    // Run ditfft2
    ditfft2(X_real, X_imag, data_in, N, 1);

    PRINTF("Serial transform complete.\n");

    // Verify results
    for(i=0;i<N;i++) {
        if(!areSame(X_real_other[i],X_real[i]) ||
        !areSame(X_imag_other[i],X_imag[i])) {
            intact = false;
            PRINTF("Mismatch at index %d\n",i);
            break;
        }
    }
    if(intact) {
        PRINTF("Program produced correct results.\n");
} else {
        PRINTF("Program output did not match!\n");
    }

    return NULL_GUID;
}

// Adds a verify EDT to the dependency tree.
// If x is in one large datablock instead of three small ones,
// pass NULL_GUID for the second and third parameters.
// Returns the guid of the EDT's completion event.
ocrGuid_t setUpVerify(ocrGuid_t inDB, ocrGuid_t XrealDB, ocrGuid_t XimagDB, u64 N, ocrGuid_t trigger) {
    ocrGuid_t vGuid,vEventGuid,vTemp, XrealVDB, XimagVDB;
    ocrEdtTemplateCreate(&vTemp, &fftVerifyEdt, 1, EDT_PARAM_UNK);
    // OCR demands the second parameter be non-null
    float *X;

    ocrDbCreate(&XrealVDB, (void**)&X, sizeof(float)*N, 0, NULL_GUID, NO_ALLOC);
    ocrDbCreate(&XimagVDB, (void**)&X, sizeof(float)*N, 0, NULL_GUID, NO_ALLOC);

    if(XimagDB == NULL_GUID) {
        ocrGuid_t verifyDependencies[4] = { inDB, XrealVDB, XimagVDB, trigger };

        ocrEdtCreate(&vGuid, vTemp, EDT_PARAM_DEF, &N, 4, verifyDependencies,
                     EDT_PROP_NONE, NULL_GUID, &vEventGuid);
    } else {
        ocrGuid_t verifyDependencies[6] = { inDB, XrealDB, XimagDB, XrealVDB,
                                            XimagVDB, trigger };

        ocrEdtCreate(&vGuid, vTemp, EDT_PARAM_DEF, &N, 6, verifyDependencies,
                     EDT_PROP_NONE, NULL_GUID, &vEventGuid);
    }

    return vEventGuid;
}
