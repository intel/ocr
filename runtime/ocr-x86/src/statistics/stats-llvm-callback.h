/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#ifndef __STATS_LLVM_CALLBACK_H__
#define __STATS_LLVM_CALLBACK_H__

#include "debug.h"
#include "ocr-task.h"
#include "ocr-types.h"

typedef struct _memTable_t {
    ocrGuid_t guid;
    void *addr;
    u64 len;
    ocrTask_t taskEdt;
    struct _memTable *next;
} memTable_t;

void _statsAddQueue(ocrGuid_t *db, void* addr, u64 len, ocrTask_t *task);

memTable_t* _statsIsAddrPresent(void* addr);

u32 _statsLoadCallback(void* address, u64 size); 
u32 _statsStoreCallback(void* address, u64 size); 
void _statsBuildAddressTableCallback(void);

#endif /* __STATS_LLVM_CALLBACK_H__ */
#endif /* OCR_ENABLE_STATISTICS */
