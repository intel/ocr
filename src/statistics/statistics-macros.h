/**
 * @brief Macros useful for defining stat processes.
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#ifndef __STATISTICS_MACROS_H__
#define __STATISTICS_MACROS_H__

#define DEBUG_TYPE STATS

#include "debug.h"
#include "ocr-statistics.h"
#include "ocr-types.h"

extern u8 intProcessMessage(ocrStatsProcess_t *dst);

#define STATS_SETUP(PROCESS, GUID)                      \
    do {                                                \
        u64 i;                                          \
        PROCESS->me = GUID;                             \
        ocrPolicyDomain_t *pd = getCurrentPD();         \
        ocrPolicyCtx_t *ctx = getCurrentWorkerContext();\
        PROCESS->processing = pd->getLock(pd, ctx);     \
        PROCESS->messages = pd->getQueue(pd, 32, ctx);  \
        PROCESS->tick = 0;                              \
        PROCESS->filters = (ocrStatsFilter_t***)        \
            checkedMalloc(sizeof(ocrStatsFilter_t**)*   \
                          (STATS_EVT_MAX+1));           \
        PROCESS->filterCounts = (u64*)checkedMalloc(    \
            sizeof(u64)*(STATS_EVT_MAX+1));             \
        for(i = 0; i < (u64)(STATS_EVT_MAX) + 1; ++i)   \
            PROCESS->filterCounts[i] = 0ULL;            \
        DPRINTF(DEBUG_LVL_INFO, "Created a StatsProcess (0x%lx) for GUID 0x%lx\n", (u64)PROCESS, GUID); \
    } while(0);
    
#define STATS_DESTRUCT(PROCESS)                         \
    do {                                                \
        u64 i, j;                                       \
        DPRINTF(DEBUG_LVL_INFO, "Destroying StatsProcess 0x%lx (GUID: 0x%lx)\n", (u64)PROCESS, PROCESS->me); \
        while(!PROCESS->processing->fctPtrs->trylock(   \
                  PROCESS->processing))                 \
        while(intProcessMessage(PROCESS)) ;             \
                                                        \
        u32 maxJ = (u32)(PROCESS->filterCounts[0] &     \
            0xFFFFFFFF);                                \
        for(j = 0; j < maxJ; ++j) {                     \
            PROCESS->filters[0][j]->fctPtrs->destruct(  \
                PROCESS->filters[0][j]);                \
        }                                               \
        if(maxJ)                                        \
            free(PROCESS->filters[0]);                  \
        for(i = 1; i < (u64)STATS_EVT_MAX + 1; ++i) {   \
            if(PROCESS->filterCounts[i])                \
                free(PROCESS->filters[i]);              \
        }                                               \
                                                        \
        free(PROCESS->filters);                         \
        free(PROCESS->filterCounts);                    \
                                                        \
        PROCESS->processing->fctPtrs->unlock(           \
            PROCESS->processing);                       \
        PROCESS->processing->fctPtrs->destruct(         \
            PROCESS->processing);                       \
                                                        \
        PROCESS->messages->fctPtrs->destruct(           \
            PROCESS->messages);                         \
    } while(0);

#endif /* __STATISTICS_MACROS_H__ */
#endif /* OCR_ENABLE_STATISTICS */
