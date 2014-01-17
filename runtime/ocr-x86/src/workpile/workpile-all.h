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
    workpileFsimMessage_id,
    workpileMax_id,
} workpileType_t;

const char * workpile_types[] = {
    "HC",
    "CE",
    "FSIM",
    NULL,
};

#ifdef ENABLE_WORKPILE_HC
#include "workpile/hc/hc-workpile.h"
#endif
#ifdef ENABLE_WORKPILE_CE
#include "workpile/ce/ce-workpile.h"
#endif

inline ocrWorkpileFactory_t * newWorkpileFactory(workpileType_t type, ocrParamList_t *perType) {
    switch(type) {
#ifdef ENABLE_WORKPILE_HC
    case workpileHc_id:
        return newOcrWorkpileFactoryHc(perType);
#endif
#ifdef ENABLE_WORKPILE_CE
    case workpileCe_id:
        return newOcrWorkpileFactoryCe(perType);
#endif
    case workpileFsimMessage_id:
    case workpileMax_id:
    default:
        ASSERT(0);
    }
    return NULL;
}

#endif /* __WORKPILE_ALL_H__ */
