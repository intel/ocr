/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_FSIM

#include "debug.h"

#include "ocr-comm-platform.h"
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
int memcpy(void * dst, void * src, u64 len)
{
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
    // HACK
    //base->comm->fcts.destruct(base->comm);
    runtimeChunkFree((u64)base, NULL);
}

void fsimCompBegin(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    
    ocrCompPlatformFsim_t * fsimCompPlatform = (ocrCompPlatformFsim_t *) compPlatform;
    compPlatform->pd = PD;

    // HACK
    compPlatform->comm->fcts.begin(compPlatform->comm, PD, workerType);
}

void fsimCompStart(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, ocrWorker_t * worker) {
    ocrCompPlatformFsim_t * fsimCompPlatform = (ocrCompPlatformFsim_t *) compPlatform;
    compPlatform->worker = worker;

    fsimRoutineExecute(worker);
}

void fsimCompStop(ocrCompPlatform_t * compPlatform) {
    // Nothing to do really
    // HACK
    //compPlatform->comm->fcts.stop(compPlatform->comm);
}

void fsimCompFinish(ocrCompPlatform_t *compPlatform) {
    // HACK
    //compPlatform->comm->fcts.finish(compPlatform->comm);
}

u8 fsimCompGetThrottle(ocrCompPlatform_t *self, u64* value) {
    return 1;
}

u8 fsimCompSetThrottle(ocrCompPlatform_t *self, u64 value) {
    return 1;
}

u8 fsimCompSendMessage(ocrCompPlatform_t *self, ocrLocation_t target,
                      ocrPolicyMsg_t **message) {
    return self->comm->fcts.sendMessage(self->comm, target, message);
}

u8 fsimCompPollMessage(ocrCompPlatform_t *self, ocrPolicyMsg_t **message, u32 mask) {
    return self->comm->fcts.pollMessage(self->comm, message, mask);
}

u8 fsimCompWaitMessage(ocrCompPlatform_t *self, ocrPolicyMsg_t **message) {
    return self->comm->fcts.waitMessage(self->comm, message);
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
    factory->initialize(factory, compPlatformFsim, perInstance);
    return compPlatformFsim;
}

void initializeCompPlatformFsim(ocrCompPlatformFactory_t * factory, ocrCompPlatformFsim_t *comPlatformFsim, ocrParamList_t *perInstance) {
    initializeCompPlatformOcr(factory, compPlatformFsim, perInstance);
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

    ocrCompPlatformFactoryFsim_t * derived = (ocrCompPlatformFactoryFsim_t *) base;

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
    base->platformFcts.sendMessage = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, ocrLocation_t, ocrPolicyMsg_t**), fsimCompSendMessage);
    base->platformFcts.pollMessage = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, ocrPolicyMsg_t**, u32), fsimCompPollMessage);
    base->platformFcts.waitMessage = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, ocrPolicyMsg_t**), fsimCompWaitMessage);
    base->platformFcts.setCurrentEnv = FUNC_ADDR(u8 (*)(ocrCompPlatform_t*, ocrPolicyDomain_t*, ocrWorker_t*), fsimCompSetCurrentEnv);

    return base;
}
#endif /* ENABLE_COMP_PLATFORM_FSIM */
