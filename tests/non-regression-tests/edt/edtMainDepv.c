/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ocr.h"

ocrGuid_t mainEdt ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 argc;
    void *programArg = depv[0].ptr;
    u64* dbAsU64 = (u64*)programArg;
    argc = dbAsU64[0];
    assert(argc == 2);

    u64 offset = dbAsU64[2];
    char *dbAsChar = (char*)programArg;
    int res = atoi(dbAsChar+offset);
    printf("In mainEdt with value %d\n", res);
    assert(res == 42);

    ocrShutdown();
    return 0;
}
