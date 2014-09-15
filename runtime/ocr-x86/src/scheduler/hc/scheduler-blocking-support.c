/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_BLOCKING_SUPPORT

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"

#define DEBUG_TYPE SCHEDULER
//TODO this should be set by configure or something
#define HELPER_MODE 1

#ifdef HELPER_MODE
static u8 masterHelper(ocrWorker_t * worker) {
    // Save current worker context
    //TODO this should be implemented in the worker
    ocrTask_t * suspendedTask = worker->curTask;
    DPRINTF(DEBUG_LVL_VERB, "Shifting worker from EDT GUID 0x%lx\n",
            suspendedTask->guid);
    // printf("MARKER2 MASTER HELPER invoked\n");
    // In helper mode, just try to execute another task
    // on top of the currently executing task's stack.
    worker->fcts.workShift(worker);

    // restore worker context
    //TODO this should be implemented in the worker
    DPRINTF(DEBUG_LVL_VERB, "Worker shifting back to EDT GUID 0x%lx\n",
            suspendedTask->guid);
    worker->curTask = suspendedTask;

    return 0;
}
#endif

/**
 * Try to ensure progress when current worker is blocked on some runtime operation
 *
 *
 */
u8 handleWorkerNotProgressing(ocrWorker_t * worker) {
    #ifdef HELPER_MODE
    return masterHelper(worker);
    #endif
}


#endif /* ENABLE_SCHEDULER_BLOCKING_SUPPORT */
