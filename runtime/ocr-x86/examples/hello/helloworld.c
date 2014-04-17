/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>

#include "ocr.h"

ocrGuid_t mainEdt ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    printf("Hello !\n");
    ocrShutdown();
    return 0;
}
