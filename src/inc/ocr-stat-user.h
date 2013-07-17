/**
 * @brief User-defined messages/filters that are used by the statistics gathering
 * framework. Add all the messages/filters you which to use here
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS

#ifndef __OCR_STAT_USER_H__
#define __OCR_STAT_USER_H__

#include "ocr-statistics.h"
#include "ocr-sync.h"

#include <stdio.h>

/**
 * @defgroup statsUserMacros Macros for defining messages
 * and filters
 *
 * Messages and filters can be defined and linked depending
 * on what statistics need to be captured. These can be
 * defined here with the help of the macros included in this
 * section
 * @{
 */

#define MESSAGE_TYPE(name) ocrSTMESS_##name##_t

/**
 * @brief Starting part of declaring a message.
 *
 * Use START_MESSAGE, followed by the additional
 * fields for the message and end with END_MESSAGE
 */
#define START_MESSAGE(name)                \
    typedef struct _ocrSTMESS_##name##_t { \
        ocrStatsMessage_t base;


/**
 * @brief Ending part of declaring a message
 *
 * @see START_MESSAGE
 */
#define END_MESSAGE(name)   \
    } ocrSTMESS_##name##_t; \
ocrStatsMessage_t* newStatsMessage_##name();

/**
 * @brief Use when a message 'name' needs to
 * be allocated and initialized
 *
 * This function should be used in the rest of the runtime
 * code
 */
#define NEW_MESSAGE(name) newStatsMessage_##name()


#define FILTER_TYPE(name) ocrSTFILT_##name##_t

/**
 * @brief Starting part of declaring a filter.
 *
 * Use START_FILTER, followed by the additional
 * fields for the filter and end with END_FILTER
 */
#define START_FILTER(name)                 \
    typedef struct _ocrSTFILT_##name##_t { \
    ocrStatsFilter_t base;

/**
 * @brief Ending part of declaring a filter
 *
 * @see START_FILTER
 */
#define END_FILTER(name)    \
    } ocrSTFILT_##name##_t; \
ocrStatsFilter_t* newStatsFilter_##name();

#define NEW_FILTER(name) newStatsFilter_##name()

/**
 * @}
 */

// A simple message (does not contain any additional information)
START_MESSAGE(simple)
END_MESSAGE(simple)

// A simple filter (simply stores the events and will re-dump them out)
typedef struct {
    u64 tick;
    ocrGuid_t src, dest;
    ocrStatsEvt_t type;
} intSimpleMessageNode_t;

START_FILTER(simple)
u64 count, maxCount;
intSimpleMessageNode_t *messages;
END_FILTER(simple)

// A simple filter which prints everything it receives out to a file
// Can be used as a very simple aggregator
// This is a *highly* inefficient implementation but is provided
// for illustration purposes;
START_FILTER(filedump)
ocrLock_t *lock;
FILE *outFile;
END_FILTER(filedump)

#undef START_MESSAGE
#undef END_MESSAGE
#undef START_FILTER
#undef END_FILTER

#endif /* __OCR_STAT_USER_H__ */

#endif /* OCR_ENABLE_STATISTICS */
