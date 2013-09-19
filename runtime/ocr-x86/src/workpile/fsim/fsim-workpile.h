#ifndef __FSIM_WORKPILE_H__
#define __FSIM_WORKPILE_H__

#include "ocr-utils.h"
#include "ocr-workpile.h"
#include "workpile/hc/deque.h"

typedef struct ce_message_workpile {
    ocrWorkpile_t base;
    semiConcDeque_t * deque;
} ocrWorkpileFsimMessage_t;

typedef struct {
    ocrWorkpileFactory_t base;
} ocrWorkpileFactoryFsimMessage_t;


ocrWorkpileFactory_t* newOcrWorkpileFactoryFsimMessage_t(ocrParamList_t *perType);

#endif //
