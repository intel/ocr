/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_COMM_WORKER_H__
#define __HC_COMM_WORKER_H__

#include "ocr-config.h"
#ifdef ENABLE_WORKER_HC_COMM

#include "ocr-types.h"
#include "utils/ocr-utils.h"
#include "utils/list.h"
#include "ocr-worker.h"

#define HC_COMM_WORKER_RL_MAX 3

typedef struct {
    ocrWorkerFactoryHc_t base;
    void (*baseInitialize) (struct _ocrWorkerFactory_t * factory,
                                  struct _ocrWorker_t *self, ocrParamList_t *perInstance);
    void (*baseStop)(struct _ocrWorker_t *self);
} ocrWorkerFactoryHcComm_t;

typedef struct {
    ocrWorkerHc_t worker;
    // cached base function pointers
    void (*baseStop)(struct _ocrWorker_t *self);
    volatile int rl;
    volatile int rl_completed [HC_COMM_WORKER_RL_MAX+1];
} ocrWorkerHcComm_t;

ocrWorkerFactory_t* newOcrWorkerFactoryHcComm(ocrParamList_t *perType);

#endif /* ENABLE_WORKER_HC_COMM */
#endif /* __HC_COMM_WORKER_H__ */
