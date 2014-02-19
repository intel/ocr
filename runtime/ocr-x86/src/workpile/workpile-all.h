/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __WORKPILE_ALL_H__
#define __WORKPILE_ALL_H__

#include "ocr-config.h"

#include "utils/ocr-utils.h"
#include "ocr-workpile.h"

typedef enum _workpileType_t {
    workpileHc_id,
    workpileCe_id,
    workpileXe_id,
    workpileFsimMessage_id,
    workpileMax_id,
} workpileType_t;

extern const char * workpile_types[];

ocrWorkpileFactory_t * newWorkpileFactory(workpileType_t type, ocrParamList_t *perType);

#endif /* __WORKPILE_ALL_H__ */
