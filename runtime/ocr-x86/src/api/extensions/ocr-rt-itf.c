/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_EXTENSION_RTITF

#include "debug.h"
#include "ocr-runtime.h"
#include "ocr-sal.h"

#warning Experimental OCR RT interface support enabled

/**
   @brief Get @ offset in the currently running edt's local storage
   Note: not visible from the ocr user interface
 **/
ocrGuid_t ocrElsUserGet(u8 offset) {
    ocrTask_t *task = NULL;
    // User indexing start after runtime-reserved ELS slots
    offset = ELS_RUNTIME_SIZE + offset;
    getCurrentEnv(NULL, NULL, &task, NULL);
    return task->els[offset];
}

/**
   @brief Set data @ offset in the currently running edt's local storage
   Note: not visible from the ocr user interface
 **/
void ocrElsUserSet(u8 offset, ocrGuid_t data) {
    // User indexing start after runtime-reserved ELS slots
    ocrTask_t *task = NULL;
    offset = ELS_RUNTIME_SIZE + offset;
    getCurrentEnv(NULL, NULL, &task, NULL);
    task->els[offset] = data;
}

ocrGuid_t currentEdtUserGet() {
    ocrTask_t *task = NULL;
    getCurrentEnv(NULL, NULL, &task, NULL);
    if(task) {
        return task->guid;
    }
    return NULL_GUID;
}

// exposed to runtime implementers as convenience
u64 ocrNbWorkers() {
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    if(pd != NULL)
        return pd->workerCount;
    return 0;
}

// exposed to runtime implementers as convenience
ocrGuid_t ocrCurrentWorkerGuid() {
    ocrWorker_t *worker = NULL;
    getCurrentEnv(NULL, &worker, NULL, NULL);
    return worker->fguid.guid;
}

// Inform the OCR runtime the currently executing thread is logically blocked
u8 ocrInformLegacyCodeBlocking() {
    // The current default implementation is to execute another EDT to try to make progress
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    return pd->schedulers[0]->fcts.monitorProgress(pd->schedulers[0], MAX_MONITOR_PROGRESS, NULL);
}

#endif /* ENABLE_EXTENSION_RTITF */
