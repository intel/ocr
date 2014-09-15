/**
 * @brief Delegation communication API
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */
#ifndef __COMM_API_DELEGATE_H__
#define __COMM_API_DELEGATE_H__

#include "ocr-config.h"
#ifdef ENABLE_COMM_API_DELEGATE

#include "ocr-comm-api.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef struct {
    ocrCommApiFactory_t base;
} ocrCommApiFactoryDelegate_t;

typedef struct {
    ocrCommApi_t base;
} ocrCommApiDelegate_t;

typedef struct {
    paramListCommApiInst_t base;
} paramListCommApiDelegate_t;

typedef struct {
    ocrMsgHandle_t handle;
    // Allows to remember the source and thus destination for the callback
    u64 boxId; // set by the scheduler
} delegateMsgHandle_t;

extern ocrCommApiFactory_t* newCommApiFactoryDelegate(ocrParamList_t *perType);

#endif /* ENABLE_COMM_API_DELEGATE */
#endif /* __COMM_API_DELEGATE__ */
