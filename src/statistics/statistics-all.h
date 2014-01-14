/**
 * @brief OCR statistics providers
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#ifndef __STATISTICS_ALL_H__
#define __STATISTICS_ALL_H__

#include "debug.h"
#include "ocr-statistics.h"
#include "ocr-utils.h"

typedef enum _statisticsType_t {
    statisticsDefault_id,
    statisticsMax_id
} statisticsType_t;

const char * statistics_types[] = {
    "default",
    NULL
};

// Default statistics provider
#include "statistics/default/default-statistics.h"

// Add other statistics provider using the same pattern as above

inline ocrStatsFactory_t *newStatsFactory(statisticsType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case statisticsDefault_id:
        return newStatsFactoryDefault(typeArg);
    case statisticsMax_id:
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __STATISTICS_ALL_H__ */

#endif /* OCR_ENABLE_STATISTICS */
