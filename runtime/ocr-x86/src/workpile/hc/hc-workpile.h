/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __HC_WORKPILE_H__
#define __HC_WORKPILE_H__

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_HC

#include "utils/ocr-utils.h"
#include "ocr-workpile.h"
#include "workpile/hc/deque.h"

typedef struct {
    ocrWorkpileFactory_t base;
} ocrWorkpileFactoryHc_t;

typedef struct {
    ocrWorkpile_t base;
    deque_t * deque;
} ocrWorkpileHc_t;

ocrWorkpileFactory_t* newOcrWorkpileFactoryHc(ocrParamList_t *perType);


#endif /* ENABLE_WORKPILE_HC */
#endif /* __HC_WORKPILE_H__ */
