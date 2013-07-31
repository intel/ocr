
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#ifndef __OCR_MESSAGES_ALL_H__
#define __OCR_MESSAGES_ALL_H__

#include "debug.h"
#include "ocr-statistics.h"

#define STATS_MESSAGE_DECLARE(name)                                              \
    extern ocrStatsFilter_t* newStatsMessage ## name(ocrStatsFilter_t *parent,   \
                                                   ocrStatsParam_t *instanceArg, \
                                                   ocrStatsFilterFcts_t *funcs)

#define STATS_FILTER_CASE(name, parent, instanceArg, funcs)                      \
    case STATS_FILTER_##name: return newStatsFilter##name (parent, instanceArg, funcs)


// Add your filter identifier here
typedef enum _ocrStatsEvtInt_t {
    STATS_MESSAGE_TRIVIAL,       /**< Trivial message that logs source and destination */
} ocrStatsEvtInt_t;


// Add your message here
STATS_MESSAGE_DECLARE(TRIVIAL);

// Finally, add your message in this switch statement
inline ocrStatsMessage_t* newStatsMessage(ocrStatsFilterType_t type,
                                          ocrStatsFilter_t *parent,
                                          ocrStatsParam_t *instanceArg,
                                          ocrStatsFilterFcts_t *funcs) {
    switch(type) {
    STATS_MESSAGE_CASE(TRIVIAL, parent, instanceArg, funcs);
    default:
        ASSERT(0);
        return NULL;
    }
}

#undef STATS_MESSAGE_DECLARE
#undef STATS_MESSAGE_CASE

#endif /* __OCR_MESSAGES_ALL_H__ */
#endif /* OCR_ENABLE_STATISTICS */

