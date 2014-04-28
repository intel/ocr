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

extern const char * commapi_types[];

#include "comm-api/handleless/handleless-comm-api.h"

// Add other communication APIs using the same pattern as above

ocrCommApiFactory_t *newCommApiFactory(commApiType_t type, ocrParamList_t *typeArg);

#endif /* __COMM_API_ALL_H__ */
