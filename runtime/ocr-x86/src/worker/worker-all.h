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

extern const char * worker_types[];

// The below is to look up ocrWorkerType_t in inc/ocr-runtime-types.h
extern const char * ocrWorkerType_types[];

#include "worker/hc/hc-worker.h"
#include "worker/ce/ce-worker.h"
#include "worker/xe/xe-worker.h"


ocrWorkerFactory_t * newWorkerFactory(workerType_t type, ocrParamList_t *perType);

#endif /* __WORKER_ALL_H__ */
