
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
    extern ocrStatsMessage_t* newStatsMessage ## name(ocrStatsEvt_t type, ocrGuid_t src, \
                                                      ocrGuid_t dest, ocrStatsParam_t *instanceArg)

#define STATS_MESSAGE_CASE(name, type, src, dest, instanceArg)           \
    case STATS_MESSAGE_##name: return newStatsMessage##name (type, src, dest, instanceArg)


// Add your filter identifier here
typedef enum _ocrStatsEvtInt_t {
    STATS_MESSAGE_TRIVIAL,       /**< Trivial message that logs source and destination */
} ocrStatsEvtInt_t;


// Add your message here
STATS_MESSAGE_DECLARE(TRIVIAL);

// Finally, add your message in this switch statement
static inline ocrStatsMessage_t* newStatsMessage(ocrStatsEvtInt_t implType, ocrStatsEvt_t type,
        ocrGuid_t src, ocrGuid_t dest,
        ocrStatsParam_t *instanceArg) {
    switch(implType) {
        STATS_MESSAGE_CASE(TRIVIAL, type, src, dest, instanceArg);
    default:
        ASSERT(0);
        return NULL;
    }
}

#undef STATS_MESSAGE_DECLARE
#undef STATS_MESSAGE_CASE

#endif /* __OCR_MESSAGES_ALL_H__ */
#endif /* OCR_ENABLE_STATISTICS */

