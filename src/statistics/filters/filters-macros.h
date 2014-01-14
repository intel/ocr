/**
 * @brief Macros useful for defining filters. This file should be included
 * in every C file defining a filter
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#ifndef __FILTERS_MACROS_H__
#define __FILTERS_MACROS_H__

#define DEBUG_TYPE STATS

#include "debug.h"
#include "ocr-statistics.h"
#include "ocr-types.h"

#define _SUBFILTER_TYPE(str) ocrSTFILT_##str##_t
#define _FILTER_TYPE(str) _SUBFILTER_TYPE(str)

#define SUBFILTER_TYPE(str) ocrSTFILT_##str##_t
#define FILTER_TYPE(str) SUBFILTER_TYPE(str)

#define SUBCOMBI(beg, end) beg ## end
#define COMBI(beg, end) SUBCOMBI(beg, end)
/**
 * @brief Starting part of declaring a filter.
 *
 * Use START_FILTER, followed by the additional
 * fields for the filter and end with END_FILTER
 */
#define START_FILTER                                    \
    typedef struct _FILTER_TYPE(FILTER_NAME) {          \
    ocrStatsFilter_t base;

/**
 * @brief Ending part of declaring a filter
 *
 * @see START_FILTER
 */
#define END_FILTER                                      \
    } FILTER_TYPE(FILTER_NAME);                      

#define FILTER_CAST(result, origin)                     \
    FILTER_TYPE(FILTER_NAME) *result = (FILTER_TYPE(FILTER_NAME) *)origin

#define FILTER_TYPE_CAST(type, result, origin)          \
    FILTER_TYPE(type) *result = (FILTER_TYPE(type) *)origin

#define FILTER_MALLOC(result)                           \
    FILTER_TYPE(FILTER_NAME) *result = (FILTER_TYPE(FILTER_NAME) *) \
        malloc(sizeof(FILTER_TYPE(FILTER_NAME)))

#define FILTER_SETUP(result, parent)                                    \
    result->base.fcts.destruct = COMBI(&_destruct_, FILTER_NAME);       \
    result->base.fcts.dump = COMBI(&_dump_, FILTER_NAME);               \
    result->base.fcts.notify = COMBI(&_notify_, FILTER_NAME);           \
    result->base.fcts.merge = COMBI(&_merge_, FILTER_NAME);             \
    result->base.parent = parent                     

#define FILTER_DESTRUCT static void COMBI(_destruct_, FILTER_NAME) (ocrStatsFilter_t *self)
#define FILTER_DUMP static u64 COMBI(_dump_, FILTER_NAME) (ocrStatsFilter_t *self, char** out, u64 chunk, \
                                                           ocrParamList_t* configuration)
#define FILTER_NOTIFY static void COMBI(_notify_, FILTER_NAME) (ocrStatsFilter_t *self, \
                                                                ocrStatsMessage_t *mess)
#define FILTER_MERGE static void COMBI(_merge_, FILTER_NAME) (ocrStatsFilter_t *self, ocrStatsFilter_t *other, \
                                                              u8 toKill)

#define FILTER_CREATE ocrStatsFilter_t* COMBI(newStatsFilter, FILTER_NAME) (ocrStatsFilter_t *parent, \
                                                                            ocrStatsParam_t *instanceArg)


#endif /* __FILTERS_MACROS_H__ */
#endif /* OCR_ENABLE_STATISTICS */
