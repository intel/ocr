/*
 * This file is subject to the license agreement located in the file LIXENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __XE_WORKPILE_H__
#define __XE_WORKPILE_H__

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_XE

#include "utils/ocr-utils.h"
#include "utils/deque.h"
#include "ocr-workpile.h"

typedef struct {
    ocrWorkpileFactory_t base;
} ocrWorkpileFactoryXe_t;

typedef struct {
    ocrWorkpile_t base;
} ocrWorkpileXe_t;

ocrWorkpileFactory_t* newOcrWorkpileFactoryXe(ocrParamList_t *perType);


#endif /* ENABLE_WORKPILE_XE */
#endif /* __XE_WORKPILE_H__ */
