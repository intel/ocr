/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#define OCR_ENABLE_STATISTICS 1
#define OCR_ENABLE_PROFILING_STATISTICS 1

#define __OCR__
#include "compat.h"

#include "ocr-stathooks.h"

#include <stdlib.h>

#define FLAGS DB_PROP_NONE

ocrGuid_t mainEdt (u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    _threadInstrumentOn = 1;
    PRINTF("Starting MainEDT\n");

    PTR_T memDb;
    ocrGuid_t memGuid;
    ocrGuid_t mem_guid, mem_affinity;

    DBCREATE(&memGuid, &memDb, sizeof(u64)*10, FLAGS, NULL_GUID, NO_ALLOC);

    int i;
    u64* buf = (u64*)malloc(10*sizeof(u64));
    for(i = 0; i < 10; i++)
        buf[i] = i;

    u64 *ptr = (u64*)memDb;
    ptr[0] = (u64)paramc;
    ptr[1] = (u64)depc;

    u64 j = ptr[3];
    ocrShutdown();
    return NULL_GUID;
}


