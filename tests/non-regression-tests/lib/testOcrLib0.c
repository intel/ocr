/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <assert.h>

#include "ocr.h"

// Only tested when OCR library interface is available
#ifdef OCR_LIBRARY_ITF

#include "ocr-lib.h"

/**
 * DESC: Simplest OCR-library, init, shutdown, finalize
 */

int main(int argc, const char * argv[]) {
    ocrConfig_t ocrConfig;
    ocrParseArgs(argc, argv, &ocrConfig);
    ocrInit(&ocrConfig);
    printf("Running\n");
    ocrShutdown();
    ocrFinalize();
    return 0;
}

#else

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrShutdown();
    return NULL_GUID;
}

#endif
