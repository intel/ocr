/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __WORKER_ALL_H__
#define __WORKER_ALL_H__

#include "ocr-utils.h"
#include "ocr-worker.h"

typedef enum _workerType_t {
    workerHc_id,
    workerFsimXE_id,
    workerFsimCE_id,
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
    case workerFsimXE_id:
    //DELME    return newOcrWorkerFactoryFsimXE(perType);
    case workerFsimCE_id:
    //DELME    return newOcrWorkerFactoryFsimCE(perType);
    case workerHc_id:
      return newOcrWorkerFactoryHc(perType);
    default:
      ASSERT(0);
    }
    return NULL;
}

#endif /* __WORKER_ALL_H__ */
