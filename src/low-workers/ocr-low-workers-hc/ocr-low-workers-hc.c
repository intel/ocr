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
#include "hc.h"

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
void hc_worker_create ( ocr_worker_t * base, void * configuration, int id) {
    hc_worker_t * hc_worker = (hc_worker_t *) base;
    hc_worker->policy_domain_guid = guidify((ocr_model_policy_t*)configuration);
    hc_worker->id = id;
}

void hc_worker_destruct ( ocr_worker_t * base ) {
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

ocrGuid_t hc_getCurrentPolicyDomain (ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    return hcWorker->policy_domain_guid;
}

ocrGuid_t hc_getCurrentEDT (ocr_worker_t * base) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    return hcWorker->currentEDT_guid;
}

void hc_setCurrentEDT (ocr_worker_t * base, ocrGuid_t curr_edt_guid) {
    hc_worker_t * hcWorker = (hc_worker_t *) base;
    hcWorker->currentEDT_guid = curr_edt_guid;
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
    hc_worker_t * worker = (hc_worker_t *) malloc(sizeof(hc_worker_t));
    worker->id = -1;
    worker->run = false;
    worker->guid = guidify((void*)worker);
    worker->currentEDT_guid = NULL_GUID;
    ocr_worker_t * base = (ocr_worker_t *) worker;
    ocr_module_t* module_base = (ocr_module_t*) base;
    module_base->map_fct = hc_ocr_module_map_scheduler_to_worker;
    base->scheduler = NULL;
    base->routine = worker_computation_routine;
    base->create = hc_worker_create;
    base->destruct = hc_worker_destruct;
    base->start = hc_start_worker;
    base->stop = hc_stop_worker;
    base->is_running = hc_is_running_worker;
    base->getCurrentPolicyDomain = hc_getCurrentPolicyDomain;
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
    hc_worker_t * hcWorker = (hc_worker_t *) worker;
    return hcWorker->guid;
}


/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

struct ocr_task_factory_struct* hc_task_factory_constructor(void) {
    hc_task_factory* derived = (hc_task_factory*) malloc(sizeof(hc_task_factory));
    ocr_task_factory* base = (ocr_task_factory*) derived;
    base->create = hc_task_factory_create;
    base->destruct =  hc_task_factory_destructor;
    return base;
}

void hc_task_factory_destructor ( struct ocr_task_factory_struct* base ) {
    hc_task_factory* derived = (hc_task_factory*) base;
    free(derived);
}

ocrGuid_t hc_task_factory_create_with_event_list (struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, event_list_t* l) {
    hc_task_t* edt = hc_task_construct_with_event_list(fctPtr, paramc, params, paramv, l);
    ocr_task_t* base = (ocr_task_t*) edt;
    return guidify(base);
}

ocrGuid_t hc_task_factory_create ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, size_t dep_l_size) {
    hc_task_t* edt = hc_task_construct(fctPtr, paramc, params, paramv, dep_l_size);
    ocr_task_t* base = (ocr_task_t*) edt;
    return guidify(base);
}

//TODO shall this be in namespace ocr-hc ?
void * worker_computation_routine(void * arg) {
    ocr_worker_t * worker = (ocr_worker_t *) arg;
    /* associate current thread with the worker */
    associate_executor_and_worker(worker);
    ocrGuid_t workerGuid = get_worker_guid(worker);
    ocr_scheduler_t * scheduler = get_worker_scheduler(worker);
    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(worker));
    while(worker->is_running(worker)) {
        ocrGuid_t taskGuid = scheduler->take(scheduler, workerGuid);
        if (taskGuid != NULL_GUID) {
            ocr_task_t* curr_task = (ocr_task_t*) deguidify(taskGuid);
            worker->setCurrentEDT(worker,taskGuid);
            curr_task->execute(curr_task);
            worker->setCurrentEDT(worker, NULL_GUID);
        }
    }
    return NULL;
}
