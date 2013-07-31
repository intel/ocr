/**
 * @brief A trivial message that logs just the source, destination and
 * event type
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#include "debug.h"
#include "statistics/messages/messages-macros.h"

#include <stdlib.h>

#define MESSAGE_NAME TRIVIAL

START_MESSAGE
END_MESSAGE

MESSAGE_DESTRUCT {
    free(self);
}

MESSAGE_DUMP {
    return NULL;
}

MESSAGE_CREATE {
    MESSAGE_MALLOC(rself);
    MESSAGE_SETUP(rself, funcs);
    return (ocrStatsMessage_t*)rself;
}

#undef MESSAGE_NAME

#endif /* OCR_ENABLE_STATISTICS */
