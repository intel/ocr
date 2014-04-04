/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_FSIM

#include "debug.h"

#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

#include "fsim-comp-platform.h"

#define DEBUG_TYPE COMP_PLATFORM


#if defined(TOOL_CHAIN_XE)
//
// Apparently, the compiler needs memcpy() as a proper function and
// cannot do without it for portable code... Hence, we need to define
// it here for XE LLVM, else we get undefined references.
//
// It's a tool-chain thing, not really hardware, system, or platform,
// but we have to stick it somewhere and the HAL and SAL are headers
// only -- hence this placement.
//
int memcpy(void * dst, void * src, u64 len) {
    __asm__ __volatile__("dma.copyregion %1, %0, %2\n\t"
                         "fence 0xF\n\t"
                         : : "r" (dst), "r" (src), "r" (len));
    return len;
}
#endif


// Ugly globals, but similar globals exist in pthread as well
// FIXME: These globals need to be moved out into their own registers

ocrPolicyDomain_t *myPD = NULL;
ocrWorker_t *myWorker = NULL;

static void * fsimRoutineExecute(ocrWorker_t * worker) {
    return worker->fcts.run(worker);
}

void fsimCompDestruct (ocrCompPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void fsimCompBegin(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    compPlatform->pd = PD;
}

void fsimCompStart(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, ocrWorker_t * worker) {
    compPlatform->worker = worker;
    fsimRoutineExecute(worker);
}

void fsimCompStop(ocrCompPlatform_t * compPlatform) {
    // Nothing to do really
}

void fsimCompFinish(ocrCompPlatform_t *compPlatform) {
}

u8 fsimCompGetThrottle(ocrCompPlatform_t *self, u64* value) {
    return 1;
}

u8 fsimCompSetThrottle(ocrCompPlatform_t *self, u64 value) {
    return 1;
}

u8 fsimCompSetCurrentEnv(ocrCompPlatform_t *self, ocrPolicyDomain_t *pd,
                         ocrWorker_t *worker) {

    myPD = pd;
    myWorker = worker;
    return 0;
}

/******************************************************/
/* OCR COMP PLATFORM FSIM FACTORY                  */
/******************************************************/

ocrCompPlatform_t* newCompPlatformFsim(ocrCompPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCompPlatformFsim_t * compPlatformFsim = (ocrCompPlatformFsim_t*)
            runtimeChunkAlloc(sizeof(ocrCompPlatformFsim_t), NULL);
    ocrCompPlatform_t *base = (ocrCompPlatform_t*)compPlatformFsim;
    factory->initialize(factory, base, perInstance);
    return base;
}

void initializeCompPlatformFsim(ocrCompPlatformFactory_t * factory, ocrCompPlatform_t *base, ocrParamList_t *perInstance) {
    initializeCompPlatformOcr(factory, base, perInstance);
}

void destructCompPlatformFactoryFsim(ocrCompPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

void getCurrentEnv(ocrPolicyDomain_t** pd, ocrWorker_t** worker,
                   ocrTask_t **task, ocrPolicyMsg_t* msg) {
    if(pd)
        *pd = myPD;
    if(worker)
        *worker = myWorker;
    if(myWorker && task)
        *task = myWorker->curTask;
    if(msg)
        msg->srcLocation = myPD->myLocation;
}

ocrCompPlatformFactory_t *newCompPlatformFactoryFsim(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
                                     runtimeChunkAlloc(sizeof(ocrCompPlatformFactoryFsim_t), (void *)1);

    base->instantiate = &newCompPlatformFsim;
    base->initialize = &initializeCompPlatformFsim;
    base->destruct = &destructCompPlatformFactoryFsim;
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrCompPlatform_t*), fsimCompDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrCompPlatform_t*, ocrPolicyDomain_t*, ocrWorkerType_t), fsimCompBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrCompPlatform_t*, ocrPolicyDomain_t*, ocrWorker_t*), fsimCompStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrCompPlatform_t*), fsimCompStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrCompPlatform_t*), fsimCompFinish);
    base->platformFcts.getThrottle = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, u64*), fsimCompGetThrottle);
    base->platformFcts.setThrottle = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, u64), fsimCompSetThrottle);
    base->platformFcts.setCurrentEnv = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, ocrPolicyDomain_t*, ocrWorker_t*), fsimCompSetCurrentEnv);

    return base;
}
#endif /* ENABLE_COMP_PLATFORM_FSIM */
