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
#include "ocr-guid.h"


/******************************************************/
/* OCR-HC WORKER                                      */
/******************************************************/

// Fwd declaration
ocrWorker_t* newWorkerHc(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance);

void destructWorkerFactoryHc(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryHc(ocrParamList_t * perType) {
    ocrWorkerFactoryHc_t* derived = (ocrWorkerFactoryHc_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryHc_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerHc;
    base->destruct =  destructWorkerFactoryHc;
    base->workerFcts.destruct = destructWorkerHc;
    base->workerFcts.start = hc_start_worker;
    base->workerFcts.stop = hc_stop_worker;
    base->workerFcts.isRunning = hc_is_running_worker;
    base->workerFcts.getCurrentEDT = hc_getCurrentEDT;
    base->workerFcts.setCurrentEDT = hc_setCurrentEDT;
    return base;
}

void destructWorkerHc ( ocrWorker_t * base ) {
    ocrGuidProvider_t * guidProvider = getCurrentPD()->guidProvider;
    guidProvider->fctPtrs->releaseGuid(guidProvider, base->guid);
    free(base);
}

void hc_start_worker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->run = true;
}

void hc_stop_worker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->run = false;
    log_worker(INFO, "Stopping worker %d\n", hcWorker->id);
}

bool hc_is_running_worker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    return hcWorker->run;
}

ocrGuid_t hc_getCurrentEDT (ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    return hcWorker->currentEDT_guid;
}

void hc_setCurrentEDT (ocrWorker_t * base, ocrGuid_t curr_edt_guid) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->currentEDT_guid = curr_edt_guid;
}

ocrGuid_t getCurrentEDT() {
    ocrGuid_t wGuid = ocr_get_current_worker_guid();
    ocrWorker_t *worker = NULL;
    deguidify(getCurrentPD(), wGuid, (u64*)&worker, NULL);
    return worker->fctPtrs->getCurrentEDT(worker);
}

/**
 * The computation worker routine that asks work to the scheduler
 */
void * worker_computation_routine(void * arg);

/**
 * Builds an instance of a HC worker
 */
ocrWorker_t* newWorkerHc (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorkerHc_t * worker = checkedMalloc(worker, sizeof(ocrWorkerHc_t));
    worker->run = false;
    worker->id = ((paramListWorkerHcInst_t*)perInstance)->worker_id;
    worker->currentEDT_guid = NULL_GUID;

    ocrWorker_t * base = (ocrWorker_t *) worker;
    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);
    base->routine = worker_computation_routine;
    base->fctPtrs = &(factory->workerFcts);
    return base;
}

// Convenient to have an id to index workers in pools
int get_worker_id(ocrWorker_t * worker) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    return hcWorker->id;
}

void * worker_computation_routine(void * arg) {
    ocrWorker_t * worker = (ocrWorker_t *) arg;
    // associate current thread with the worker
    associate_comp_platform_and_worker(worker);
    ocrGuid_t workerGuid = worker->guid;
    //TODO assume someone did set that beforehand or shall we fetch it from mappable or something ?
    ocrPolicyDomain_t * pd = getCurrentPD();
    // Setting up this worker context to takeEdts
    // This assumes workers are not relocatable
    ocrPolicyCtxFactory_t * ctxFactory = pd->contextFactory;
    ocrPolicyCtx_t * contextTake = ctxFactory->instantiate(ctxFactory);
    contextTake->sourcePD = pd->guid;
    contextTake->sourceObj = workerGuid;
    contextTake->sourceId = get_worker_id(worker);
    contextTake->destPD = pd->guid;
    contextTake->destObj = NULL_GUID;
    contextTake->type = PD_MSG_EDT_TAKE;
    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(worker));
    // Entering the worker loop
    while(worker->fctPtrs->isRunning(worker)) {
        ocrGuid_t taskGuid;
        u32 count;
        pd->takeEdt(pd, NULL, &count, &taskGuid, contextTake);
        ASSERT(count == 1); // remove that when we can take a bunch
        if (taskGuid != NULL_GUID) {
            ocrTask_t* curr_task = NULL;
            deguidify(pd, taskGuid, (u64*)&(curr_task), NULL);
            worker->fctPtrs->setCurrentEDT(worker,taskGuid);
            curr_task->fctPtrs->execute(curr_task);
            worker->fctPtrs->setCurrentEDT(worker, NULL_GUID);
            curr_task->fctPtrs->destruct(curr_task);
        }
    }
    return NULL;
}
