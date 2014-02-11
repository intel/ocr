/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMM_API_ALL_H__
#define __COMM_API_ALL_H__

#include "debug.h"
#include "ocr-comm-api.h"
#include "ocr-config.h"
#include "utils/ocr-utils.h"

typedef enum _commApiType_t {
    commApiHandleless_id,
    commApiMax_id
} commApiType_t;

const char * commapi_types[] = {
    "Handleless",
    NULL,
};

#include "comm-api/handleless/handleless-comm-api.h"

// Add other communication APIs using the same pattern as above

inline ocrCommApiFactory_t *newCommApiFactory(commApiType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_COMM_API_HANDLELESS
    case commApiHandleless_id:
        return newCommApiFactoryHandleless(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __COMM_API_ALL_H__ */
