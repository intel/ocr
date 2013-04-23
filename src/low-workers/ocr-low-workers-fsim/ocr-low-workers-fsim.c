/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <pthread.h>
#include <stdio.h>

#include "ocr-runtime.h"
#include "fsim.h"
#include "ocr-guid.h"

/******************************************************/
/* OCR-WORKER                                         */
/******************************************************/

void xe_stop_worker(ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    hcWorker->run = false;

    xe_worker_t * xeWorker = (xe_worker_t *) base;
    pthread_mutex_lock( &xeWorker->isRunningMutex );
    pthread_cond_signal ( &xeWorker->isRunningCond );
    pthread_mutex_unlock( &xeWorker->isRunningMutex );

    log_worker(INFO, "Stopping worker %d\n", hcWorker->id);
}

void xe_worker_create ( ocr_worker_t * base, void * per_type_configuration, void * per_instance_configuration) {
    xe_worker_t * xeWorker = (xe_worker_t *) base;

    hc_worker_t * hcWorker = &(xeWorker->hcBase);
    hcWorker->id = ((worker_configuration*)per_instance_configuration)->worker_id;

    pthread_cond_init( &xeWorker->isRunningCond, NULL);
    pthread_mutex_init( &xeWorker->isRunningMutex, NULL);
}

void xe_worker_destruct ( ocr_worker_t * base ) {
    xe_worker_t * xeWorker = (xe_worker_t *) base;
    pthread_cond_destroy( &xeWorker->isRunningCond );
    pthread_mutex_destroy( &xeWorker->isRunningMutex );

    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(base);
}

extern void* xe_worker_computation_routine (void * arg);
extern void* ce_worker_computation_routine (void * arg);

ocr_worker_t* xe_worker_constructor () {
    xe_worker_t * xeWorker = (xe_worker_t *) malloc(sizeof(xe_worker_t));
    hc_worker_t * hcWorker = &(xeWorker->hcBase);
    hcWorker->id = -1;
    hcWorker->run = false;

    hcWorker->currentEDT_guid = NULL_GUID;

    ocr_worker_t * base = (ocr_worker_t *) hcWorker;
    ocr_module_t* module_base = (ocr_module_t*) base;
    module_base->map_fct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = xe_worker_computation_routine;
    base->create = xe_worker_create;
    base->destruct = xe_worker_destruct;
    base->start = hc_start_worker;
    base->stop = xe_stop_worker;
    base->is_running = hc_is_running_worker;
    base->getCurrentEDT = hc_getCurrentEDT;
    base->setCurrentEDT = hc_setCurrentEDT;
    return base;
}

ocr_worker_t* ce_worker_constructor () {
    hc_worker_t * worker = (hc_worker_t *) malloc(sizeof(hc_worker_t));
    worker->id = -1;
    worker->run = false;

    worker->currentEDT_guid = NULL_GUID;

    ocr_worker_t * base = (ocr_worker_t *) worker;
    ocr_module_t* module_base = (ocr_module_t*) base;
    module_base->map_fct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = ce_worker_computation_routine;
    base->create = hc_worker_create;
    base->destruct = hc_worker_destruct;
    base->start = hc_start_worker;
    base->stop = hc_stop_worker;
    base->is_running = hc_is_running_worker;
    base->getCurrentEDT = hc_getCurrentEDT;
    base->setCurrentEDT = hc_setCurrentEDT;
    return base;
}
