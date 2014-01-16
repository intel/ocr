/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __WORKER_ALL_H__
#define __WORKER_ALL_H__

#include "ocr-config.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

typedef enum _workerType_t {
    workerHc_id,
    workerXe_id,
    workerCe_id,
    workerMax_id,
} workerType_t;

const char * worker_types[] = {
    "HC",
    "XE",
    "CE",
    NULL
};

#include "worker/hc/hc-worker.h"


inline ocrWorkerFactory_t * newWorkerFactory(workerType_t type, ocrParamList_t *perType) {
    switch(type) {
#ifdef ENABLE_WORKER_XE
    case workerXe_id:
      return newOcrWorkerFactoryXe(perType);
#endif
#ifdef ENABLE_WORKER_CE
    case workerCe_id:
      return newOcrWorkerFactoryCe(perType);
#endif
#ifdef ENABLE_WORKER_HC
    case workerHc_id:
      return newOcrWorkerFactoryHc(perType);
#endif
    default:
      ASSERT(0);
    }
    return NULL;
}

#endif /* __WORKER_ALL_H__ */
