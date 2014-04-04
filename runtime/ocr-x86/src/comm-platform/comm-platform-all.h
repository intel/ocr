/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __COMM_PLATFORM_ALL_H__
#define __COMM_PLATFORM_ALL_H__

#include "debug.h"
#include "ocr-comm-platform.h"
#include "ocr-config.h"
#include "utils/ocr-utils.h"

typedef enum _commPlatformType_t {
    commPlatformNull_id,
    commPlatformCe_id,
    commPlatformXe_id,
    commPlatformCePthread_id,
    commPlatformXePthread_id,
    commPlatformMax_id
} commPlatformType_t;

extern const char * commplatform_types[];

// NULL communication platform
#include "comm-platform/null/null-comm-platform.h"
#include "comm-platform/ce/ce-comm-platform.h"
#include "comm-platform/xe/xe-comm-platform.h"
#include "comm-platform/ce-pthread/ce-pthread-comm-platform.h"
#include "comm-platform/xe-pthread/xe-pthread-comm-platform.h"

// Add other communication platforms using the same pattern as above

ocrCommPlatformFactory_t *newCommPlatformFactory(commPlatformType_t type, ocrParamList_t *typeArg);

#endif /* __COMM_PLATFORM_ALL_H__ */
