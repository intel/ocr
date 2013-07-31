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

#ifndef __MESSAGES_MACROS_H__
#define __MESSAGES_MACROS_H__

#define DEBUG_TYPE STATS

#include "debug.h"
#include "ocr-statistics.h"
#include "ocr-types.h"

#define _SUBMESS_TYPE(str) ocrSTMESS_##str##_t
#define _MESS_TYPE(str) _SUBMESS_TYPE(str)

#define SUBMESS_TYPE(str) ocrSTMESS_##str##_t
#define MESS_TYPE(str) SUBMESS_TYPE(str)

#define SUBCOMBI(beg, end) beg ## end
#define COMBI(beg, end) SUBCOMBI(beg, end)
/**
 * @brief Starting part of declaring a message
 *
 * Use START_MESSAGE, followed by the additional
 * fields for the message and end with END_MESSAGE
 */
#define START_MESSAGE                                   \
    typedef struct _MESS_TYPE(MESSAGE_NAME) {           \
    ocrStatsMessage_t base;

/**
 * @brief Ending part of declaring a message
 *
 * @see START_MESSAGE
 */
#define END_MESSAGE                                     \
    } MESS_TYPE(MESSAGE_NAME);                      

#define MESSAGE_CAST(result, origin)                    \
    MESS_TYPE(MESSAGE_NAME) *result = (MESS_TYPE(MESSAGE_NAME) *)origin

#define MESSAGE_MALLOC(result)                          \
    MESS_TYPE(MESSAGE_NAME) *result = (MESS_TYPE(MESSAGE_NAME) *)       \
        malloc(sizeof(MESS_TYPE(MESSAGE_NAME)))

#define MESSAGE_SETUP(result, fPtrs)                            \
    if(fPtrs) {                                                 \
        result->base.fctPtrs = fPtrs;                           \
    } else {                                                    \
        fPtrs->destruct = COMBI(&_destruct_, MESSAGE_NAME);     \
        fPtrs->dump = COMBI(&_dump_, MESSAGE_NAME);             \
        result->base.fctPtrs = fPtrs;                           \
    }                                                           \
    result->base.tick = tick;                                   \
    result->base.src = src;                                     \
    result->base.dest = dest;                                   \
    result->base.type = type;                                   \
    result->base.state = 0;
        
    


#define MESSAGE_DESTRUCT static void COMBI(_destruct_, MESSAGE_NAME) (ocrStatsMessage_t *self)
#define MESSAGE_DUMP static char* COMBI(_dump_, MESSAGE_NAME) (ocrStatsMessage_t *self)

#define MESSAGE_CREATE ocrStatsMessage_t* COMBI(newStatsMessage, MESSAGE_NAME) (ocrStatsEvt_t type, \
                                                                                u64 tick, ocrGuid_t src, \
                                                                                ocrGuid_t dest, \
                                                                                ocrStatsParam_t *instanceArg, \
                                                                                ocrStatsMessageFcts_t *funcs)
#endif /* __MESSAGES_MACROS_H__ */
#endif /* OCR_ENABLE_STATISTICS */
