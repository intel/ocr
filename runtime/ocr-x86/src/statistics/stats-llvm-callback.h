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

/*
typedef struct _memTable_t {
    ocrGuid_t guid;
    void *addr;
    u64 len;
    ocrTask_t taskEdt;
    struct _memTable *next;
} memTable_t;

void _statsAddQueue(ocrGuid_t *db, void* addr, u64 len, ocrTask_t *task);

memTable_t* _statsIsAddrPresent(void* addr);
*/

extern void PROFILER_ocrStatsLoadCallback(void* address, u64 size, u64 instrCount); 
extern void PROFILER_ocrStatsStoreCallback(void* address, u64 size, u64 instrCount); 

#endif /* __STATS_LLVM_CALLBACK_H__ */
#endif /* OCR_ENABLE_PROFILING_STATISTICS */
#endif /* OCR_ENABLE_STATISTICS */
