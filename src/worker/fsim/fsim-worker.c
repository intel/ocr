/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <pthread.h>
#include <stdio.h>

#include "ocr-runtime.h"
#include "fsim.h"
#include "ocr-guid.h"

/******************************************************/
/* OCR-FSIM-XE WORKER                                 */
/******************************************************/

// Fwd declaration
ocrWorker_t* newWorkerFsimXE(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance);

void destructWorkerFactoryFsimXE(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryFsimXE(ocrParamList_t * perType) {
    ocrWorkerFactoryFsimXE_t* derived = (ocrWorkerFactoryFsimXE_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryFsimXE_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerFsimXE;
    base->destruct = destructWorkerFactoryFsimXE;
    return base;
}

void xe_stop_worker(ocrWorker_t * base) {
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) base;
    hcWorker->run = false;

    ocrWorkerFsimXE_t * xeWorker = (ocrWorkerFsimXE_t *) base;
    pthread_mutex_lock( &xeWorker->isRunningMutex );
    pthread_cond_signal ( &xeWorker->isRunningCond );
    pthread_mutex_unlock( &xeWorker->isRunningMutex );

    log_worker(INFO, "Stopping worker %d\n", hcWorker->id);
}

void xe_worker_destruct ( ocrWorker_t * base ) {
    ocrWorkerFsimXE_t * xeWorker = (ocrWorkerFsimXE_t *) base;
    pthread_cond_destroy( &xeWorker->isRunningCond );
    pthread_mutex_destroy( &xeWorker->isRunningMutex );

    ocrGuidProvider_t * guidProvider = getCurrentPD()->guidProvider;
    guidProvider->fctPtrs->releaseGuid(guidProvider, base->guid);
    free(base);
}

extern void* xe_worker_computation_routine (void * arg);

ocrWorker_t* newWorkerFsimXE (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    // Piggy-back on HC worker, modifying some of the implementation function
    ocrWorkerFsimXE_t * xeWorker = (ocrWorkerFsimXE_t *) malloc(sizeof(ocrWorkerFsimXE_t));
    ocrWorkerHc_t * hcWorker = &(xeWorker->hcBase);
    hcWorker->run = false;
    hcWorker->id = ((paramListWorkerFsimInst_t*)perInstance)->worker_id;
    hcWorker->currentEDTGuid = NULL_GUID;

    ocrWorker_t * base = (ocrWorker_t *) hcWorker;
    ocrMappable_t* module_base = (ocrMappable_t*) base;
    module_base->mapFct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = xe_worker_computation_routine;
    base->fctPtrs = &(factory->workerFcts);
    //TODO these need to be moved to the factory schedulerFcts
    base->fctPtrs->destruct = xe_worker_destruct;
    base->fctPtrs->start = hc_start_worker;
    base->fctPtrs->stop = xe_stop_worker;
    base->fctPtrs->isRunning = hc_is_running_worker;
    base->fctPtrs->getCurrentEDT = hc_getCurrentEDT;
    base->fctPtrs->setCurrentEDT = hc_setCurrentEDT;
    //TODO END

    pthread_cond_init( &xeWorker->isRunningCond, NULL);
    pthread_mutex_init( &xeWorker->isRunningMutex, NULL);
    return base;
}


/******************************************************/
/* OCR-FSIM-CE WORKER                                 */
/******************************************************/

// Fwd declaration
ocrWorker_t* newWorkerFsimCE(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance);

void destructWorkerFactoryFsimCE(ocrWorkerFactory_t * factory) {
    free(factory);
}

ocrWorkerFactory_t * newOcrWorkerFactoryFsimCE(ocrParamList_t * perType) {
    ocrWorkerFactoryFsimCE_t* derived = (ocrWorkerFactoryFsimCE_t*) checkedMalloc(derived, sizeof(ocrWorkerFactoryFsimCE_t));
    ocrWorkerFactory_t* base = (ocrWorkerFactory_t*) derived;
    base->instantiate = newWorkerFsimCE;
    base->destruct =  destructWorkerFactoryFsimCE;
    return base;
}

extern void* ce_worker_computation_routine (void * arg);

ocrWorker_t* newWorkerFsimCE (ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    // Piggy-back on HC worker, modifying the routine
    ocrWorkerHc_t * worker = (ocrWorkerHc_t *) malloc(sizeof(ocrWorkerHc_t));
    worker->run = false;
    worker->id = ((paramListWorkerFsimInst_t*)perInstance)->worker_id;
    worker->currentEDTGuid = NULL_GUID;

    ocrWorker_t * base = (ocrWorker_t *) worker;
    ocrMappable_t* module_base = (ocrMappable_t*) base;
    module_base->mapFct = hc_ocr_module_map_scheduler_to_worker;

    base->guid = UNINITIALIZED_GUID;
    guidify(getCurrentPD(), (u64)base, &(base->guid), OCR_GUID_WORKER);

    base->scheduler = NULL;
    base->routine = ce_worker_computation_routine;
    base->fctPtrs = (ocrWorkerFcts_t*) malloc(sizeof(ocrWorkerFcts_t));
    base->fctPtrs->destruct = destructWorkerHc;
    base->fctPtrs->start = hc_start_worker;
    base->fctPtrs->stop = hc_stop_worker;
    base->fctPtrs->isRunning = hc_is_running_worker;
    base->fctPtrs->getCurrentEDT = hc_getCurrentEDT;
    base->fctPtrs->setCurrentEDT = hc_setCurrentEDT;

    return base;
}
