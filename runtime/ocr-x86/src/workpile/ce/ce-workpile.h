/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __CE_WORKPILE_H__
#define __CE_WORKPILE_H__

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_CE

#include "utils/ocr-utils.h"
#include "utils/deque.h"
#include "ocr-workpile.h"

typedef struct {
    ocrWorkpileFactory_t base;
} ocrWorkpileFactoryCe_t;

typedef struct {
    ocrWorkpile_t base;
    deque_t * deque;
} ocrWorkpileCe_t;

ocrWorkpileFactory_t* newOcrWorkpileFactoryCe(ocrParamList_t *perType);


#endif /* ENABLE_WORKPILE_CE */
#endif /* __CE_WORKPILE_H__ */
