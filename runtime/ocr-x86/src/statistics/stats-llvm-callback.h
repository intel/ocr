/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS
#ifdef OCR_ENABLE_PROFILING_STATISTICS
#ifndef __STATS_LLVM_CALLBACK_H__
#define __STATS_LLVM_CALLBACK_H__

#include "debug.h"
#include "ocr-task.h"
#include "ocr-types.h"

typedef struct _dbTable_t {
    ocrDataBlock_t *db;
    u64 start;
    u64 end;
} dbTable_t;

typedef struct _edtTable_t {
    ocrTask_t *task;
    dbTable_t *dbList;
    int numDbs;
    int maxDbs;
} edtTable_t;

void ocrStatsAccessInsertDB(ocrTask_t *task, ocrDataBlock_t *db);
void ocrStatsAccessRemoveEDT(ocrTask_t *task);

extern void PROFILER_ocrStatsLoadCallback(void* address, u64 size, u64 instrCount, u64 fpInstrCount); 
extern void PROFILER_ocrStatsStoreCallback(void* address, u64 size, u64 instrCount, u64 fpInstrCount); 

#endif /* __STATS_LLVM_CALLBACK_H__ */
#endif /* OCR_ENABLE_PROFILING_STATISTICS */
#endif /* OCR_ENABLE_STATISTICS */
