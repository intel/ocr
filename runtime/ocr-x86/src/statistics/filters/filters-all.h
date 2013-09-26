
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#ifndef __OCR_FILTERS_ALL_H__
#define __OCR_FILTERS_ALL_H__

#include "debug.h"
#include "ocr-statistics.h"

#define STATS_FILTER_DECLARE(name)                                               \
    extern ocrStatsFilter_t* newStatsFilter ## name (ocrStatsFilter_t *parent,   \
                                                   ocrStatsParam_t *instanceArg)

#define STATS_FILTER_CASE(name, parent, instanceArg)                      \
    case STATS_FILTER_##name: return newStatsFilter ## name (parent, instanceArg)


// Add your filter identifier here
typedef enum _ocrStatsFilterType_t {
    STATS_FILTER_TRIVIAL,       /**< Trivial filter that keeps everything */
    STATS_FILTER_FILE_DUMP      /**< Trivial filter that dumps everything to a file */
} ocrStatsFilterType_t;


// Add your filter here
STATS_FILTER_DECLARE(TRIVIAL);
STATS_FILTER_DECLARE(FILE_DUMP);

// Finally, add your filter in this switch statement
static inline ocrStatsFilter_t* newStatsFilter(ocrStatsFilterType_t type,
                                               ocrStatsFilter_t *parent,
                                               ocrStatsParam_t *instanceArg) {
    switch(type) {
    STATS_FILTER_CASE(TRIVIAL, parent, instanceArg);
    STATS_FILTER_CASE(FILE_DUMP, parent, instanceArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

#undef STATS_FILTER_DECLARE
#undef STATS_FILTER_CASE

#endif /* __OCR_FILTERS_ALL_H__ */
#endif /* OCR_ENABLE_STATISTICS */
