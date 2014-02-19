/*
 * This file is subject to the license agreement located in the file LIXENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_XE

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime-types.h"
#include "ocr-sysboot.h"
#include "ocr-workpile.h"
#include "scheduler/xe/xe-scheduler.h"

/******************************************************/
/* OCR-XE SCHEDULER                                   */
/******************************************************/

void xeSchedulerDestruct(ocrScheduler_t * self) {
}

void xeSchedulerBegin(ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
}

void xeSchedulerStart(ocrScheduler_t * self, ocrPolicyDomain_t * PD) {
}

void xeSchedulerStop(ocrScheduler_t * self) {
}

void xeSchedulerFinish(ocrScheduler_t *self) {
}

u8 xeSchedulerTake(ocrScheduler_t *self, u32 *count, ocrFatGuid_t *edts) {
    return 0;
}

u8 xeSchedulerGive(ocrScheduler_t* base, u32* count, ocrFatGuid_t* edts) {
    return 0;
}

u8 xeSchedulerTakeComm(ocrScheduler_t *self, u32* count, ocrFatGuid_t* handlers, u32 properties) {
    return OCR_ENOSYS;
}

u8 xeSchedulerGiveComm(ocrScheduler_t *self, u32* count, ocrFatGuid_t* handlers, u32 properties) {
    return OCR_ENOSYS;
}

ocrScheduler_t* newSchedulerXe(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrSchedulerXe_t* derived = (ocrSchedulerXe_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerXe_t), NULL);
    
    ocrScheduler_t* base = (ocrScheduler_t*)derived;
    base->fguid.guid = UNINITIALIZED_GUID;
    base->fguid.metaDataPtr = base;
    base->pd = NULL;
    base->workpiles = NULL;
    base->workpileCount = 0;
    base->fcts = factory->schedulerFcts;
    
    return base;
}

void destructSchedulerFactoryXe(ocrSchedulerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryXe(ocrParamList_t *perType) {
    ocrSchedulerFactoryXe_t* derived = (ocrSchedulerFactoryXe_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerFactoryXe_t), NULL);
    
    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = &newSchedulerXe;
    base->destruct = &destructSchedulerFactoryXe;
    base->schedulerFcts.begin = FUNC_ADDR(void (*)(ocrScheduler_t*, ocrPolicyDomain_t*), xeSchedulerBegin);
    base->schedulerFcts.start = FUNC_ADDR(void (*)(ocrScheduler_t*, ocrPolicyDomain_t*), xeSchedulerStart);
    base->schedulerFcts.stop = FUNC_ADDR(void (*)(ocrScheduler_t*), xeSchedulerStop);
    base->schedulerFcts.finish = FUNC_ADDR(void (*)(ocrScheduler_t*), xeSchedulerFinish);
    base->schedulerFcts.destruct = FUNC_ADDR(void (*)(ocrScheduler_t*), xeSchedulerDestruct);
    base->schedulerFcts.takeEdt = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*), xeSchedulerTake);
    base->schedulerFcts.giveEdt = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*), xeSchedulerGive);
    base->schedulerFcts.takeComm = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*, u32), xeSchedulerTakeComm);
    base->schedulerFcts.giveComm = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*, u32), xeSchedulerGiveComm);
    return base;
}

#endif /* ENABLE_SCHEDULER_XE */
