/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __FSIM_WORKER_H__
#define __FSIM_WORKER_H__

#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-worker.h"
#include "task/fsim/fsim-task.h"

#include <pthread.h>

typedef struct {
    ocrWorkerFactory_t base;
} ocrWorkerFactoryCE_t;

typedef struct {
    ocrWorkerFactory_t base;
} ocrWorkerFactoryXE_t;

typedef struct _paramListWorkerCEInst_t {
    paramListWorkerInst_t base;
    u32 workerId;
} paramListWorkerCEInst_t;

typedef struct _paramListWorkerXEInst_t {
    paramListWorkerInst_t base;
    u32 workerId;
} paramListWorkerXEInst_t;

typedef struct {
    ocrWorker_t worker;
    u32 id;
    bool run;
    ocrGuid_t currentEDTGuid;
} ocrWorkerCE_t;

typedef struct {
    ocrWorker_t worker;
    u32 id;
    bool run;
    ocrGuid_t currentEDTGuid;

    pthread_cond_t isRunningCond;
    pthread_mutex_t isRunningMutex;

} ocrWorkerXE_t;

ocrWorkerFactory_t* newOcrWorkerFactoryCE(ocrParamList_t *perType);

ocrWorkerFactory_t* newOcrWorkerFactoryXE(ocrParamList_t *perType);

#endif /* __FSIM_WORKER_H__ */

