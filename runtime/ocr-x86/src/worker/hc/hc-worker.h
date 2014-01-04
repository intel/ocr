/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_WORKER_H__
#define __HC_WORKER_H__

#include "ocr-config.h"
#ifdef ENABLE_WORKER_HC

#include "ocr-types.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

typedef struct {
    ocrWorkerFactory_t base;
} ocrWorkerFactoryHc_t;

typedef struct _paramListWorkerHcInst_t {
    paramListWorkerInst_t base;
    u64 workerId;
} paramListWorkerHcInst_t;

typedef struct {
    ocrWorker_t worker;
    // The HC implementation relies on integer ids to
    // map workers, schedulers and workpiles together
    u64 id;
    // Flag the worker checksto now if he's running
    bool run;
} ocrWorkerHc_t;

ocrWorkerFactory_t* newOcrWorkerFactoryHc(ocrParamList_t *perType);

#endif /* ENABLE_WORKER_HC */
#endif /* __HC_WORKER_H__ */
