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

#include "ocr-macros.h"
#include "ocr-runtime.h"
#include "hc.h"
#include "ocr-guid.h"

/******************************************************/
/* OCR-WORKER                                         */
/******************************************************/

ocr_scheduler_t * get_worker_scheduler(ocr_worker_t * worker) { return worker->scheduler; }

/******************************************************/
/* OCR-HC WORKER                                      */
/******************************************************/

/**
 * Configure the HC worker instance
 */
void hc_worker_create ( ocr_worker_t * base, void * per_type_configuration, void * per_instance_configuration) {
    hc_worker_t * hc_worker = (hc_worker_t *) base;
    hc_worker->id = ((worker_configuration*)per_instance_configuration)->worker_id;
}

void hc_worker_destruct ( ocr_worker_t * base ) {
    globalGuidProvider->releaseGuid(globalGuidProvider, base->guid);
    free(base);
}

void hc_start_worker(ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    hcWorker->run = true;
}

void hc_stop_worker(ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    hcWorker->run = false;
    log_worker(INFO, "Stopping worker %d\n", hcWorker->id);
}

bool hc_is_running_worker(ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    return hcWorker->run;
}

ocrGuid_t hc_getCurrentEDT (ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    return hcWorker->currentEDT_guid;
}

void hc_setCurrentEDT (ocr_worker_t * base, ocrGuid_t curr_edt_guid) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    hcWorker->currentEDT_guid = curr_edt_guid;
}

ocrGuid_t getCurrentEDT() {
    ocrGuid_t wGuid = ocr_get_current_worker_guid();
    ocr_worker_t *worker = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wGuid, (u64*)&worker, NULL);
    return worker->getCurrentEDT(worker);
}

void hc_ocr_module_map_scheduler_to_worker(void * self_module, ocr_module_kind kind,
                                           size_t nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_SCHEDULER);
    assert(nb_instances == 1);
    ocr_worker_t * worker = (ocr_worker_t *) self_module;
    worker->scheduler = ((ocr_scheduler_t **) ptr_instances)[0];
}

/**
 * Builds an instance of a HC worker
 */
ocr_worker_t* hc_worker_constructor () {
    hc_worker_t * worker = checked_malloc(worker, sizeof(hc_worker_t));
    worker->id = -1;
    worker->run = false;

    worker->currentEDT_guid = NULL_GUID;

    ocr_worker_t * base = (ocr_worker_t *) worker;
    ocr_module_t* module_base = (ocr_module_t*) base;
    module_base->map_fct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    globalGuidProvider->getGuid(globalGuidProvider, &(base->guid), (u64)base, OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = worker_computation_routine;
    base->create = hc_worker_create;
    base->destruct = hc_worker_destruct;
    base->start = hc_start_worker;
    base->stop = hc_stop_worker;
    base->is_running = hc_is_running_worker;
    base->getCurrentEDT = hc_getCurrentEDT;
    base->setCurrentEDT = hc_setCurrentEDT;
    return base;
}

//TODO add this as function pointer or not ?
// Convenient to have an id to index workers in pools
int get_worker_id(ocr_worker_t * worker) {
    hc_worker_t * hcWorker = (hc_worker_t *) worker;
    return hcWorker->id;
}

ocrGuid_t get_worker_guid(ocr_worker_t * worker) {
    return worker->guid;
}

/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

//TODO shall this be in namespace ocr-hc ?
void * worker_computation_routine(void * arg) {
    ocr_worker_t * worker = (ocr_worker_t *) arg;
    /* associate current thread with the worker */
    associate_comp_platform_and_worker(worker);
    ocrGuid_t workerGuid = get_worker_guid(worker);
    ocr_scheduler_t * scheduler = get_worker_scheduler(worker);
    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(worker));
    while(worker->is_running(worker)) {
        ocrGuid_t taskGuid = scheduler->take(scheduler, workerGuid);
        if (taskGuid != NULL_GUID) {
            ocr_task_t* curr_task = NULL;
            globalGuidProvider->getVal(globalGuidProvider, taskGuid, (u64*)&(curr_task), NULL);
            worker->setCurrentEDT(worker,taskGuid);
            curr_task->fct_ptrs->execute(curr_task);
            worker->setCurrentEDT(worker, NULL_GUID);
            curr_task->destruct(curr_task);
        }
    }
    return NULL;
}
