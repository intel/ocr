/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_PTHREAD

#include "debug.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "ocr-utils.h"
#include "ocr-worker.h"

#include "pthread-comp-platform.h"

#include <pthread.h>

#define DEBUG_TYPE COMP_PLATFORM

/**
 * @brief Structure stored on a per-thread basis to keep track of
 * "who we are"
 */
typedef struct {
    ocrGuid_t compTarget;
    ocrPolicyDomain_t * pd;
} perThreadStorage_t;


/**
 * The key we use to be able to find which compPlatform we are on
 */
pthread_key_t selfKey;
pthread_once_t selfKeyInitialized = PTHREAD_ONCE_INIT;

static void * pthreadRoutineExecute(launchArg_t * launchArg) {
    return launchArg->routine(launchArg);
}

static void * pthreadRoutineWrapper(void * arg) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) arg;
    s32 cpuBind = pthreadCompPlatform->binding;
    if(cpuBind != -1) {
        DPRINTF(DEBUG_LVL_INFO, "Binding comp-platform to cpu_id %d\n", cpuBind);
        bind_thread(cpuBind);
    }
    launchArg_t * launchArg = (launchArg_t *) pthreadCompPlatform->launchArg;
    // Wrapper routine to allow initialization of local storage
    // before entering the worker routine.
    // TODO: Fixme
    perThreadStorage_t *data = (perThreadStorage_t*)checkedMalloc(data, sizeof(perThreadStorage_t));
    RESULT_ASSERT(pthread_setspecific(selfKey, data), ==, 0);
    if (launchArg != NULL) {
        return pthreadRoutineExecute(launchArg);
    }
    // When launchArgs is NULL, it is the master thread executing and it falls-through
    return NULL;
}

static void destroyKey(void* arg) {
    runtimeChunkFree((u64)arg, NULL);
}

static void initializeKey() {
    RESULT_ASSERT(pthread_key_create(&selfKey, &destroyKey), ==, 0);
    // We are going to set our own key (we are the master thread)
    perThreadStorage_t *data = (perThreadStorage_t*)runtimeChunkAlloc(sizeof(perThreadStorage_t), NULL);
    RESULT_ASSERT(pthread_setspecific(selfKey, data), ==, 0);
}

void pthreadDestruct (ocrCompPlatform_t * base) {
    // TODO: Check this
    runtimeChunkFree((u64)(((ocrCompPlatformPthread_t*)base)->launchArg), NULL);
    runtimeChunkFree((u64)base, NULL);
}

void pthreadStart(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    compPlatform->pd = PD;
    pthreadCompPlatform->launchArg = launchArg;
    pthread_attr_t attr;
    RESULT_ASSERT(pthread_attr_init(&attr), ==, 0);
    //Note this call may fail if the system doesn't like the stack size asked for.
    RESULT_ASSERT(pthread_attr_setstacksize(&attr, pthreadCompPlatform->stackSize), ==, 0);
    RESULT_ASSERT(pthread_create(&(pthreadCompPlatform->osThread),
                                 &attr, &pthreadRoutineWrapper,
                                 pthreadCompPlatform), ==, 0);
}

void pthreadStop(ocrCompPlatform_t * compPlatform) {
    // This code must be called by thread '0' to join on other threads
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    RESULT_ASSERT(pthread_join(pthreadCompPlatform->osThread, NULL), ==, 0);
}

void pthreadFinish(ocrCompPlatform_t *compPlatform) {
    // Nothing to do
}

void pthreadStartMaster(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    // This comp-platform represent the currently executing master thread.
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    //pthreadCompPlatform->launchArg = NULL;
    //pthreadRoutineWrapper(pthreadCompPlatform);
    pthreadCompPlatform->launchArg = launchArg;
}

void pthreadStopMaster(ocrCompPlatform_t * compPlatform) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    pthreadRoutineExecute(pthreadCompPlatform->launchArg);
}

u8 pthreadGetThrottle(ocrCompPlatform_t *self, u64* value) {
    return 1;
}

u8 pthreadSetThrottle(ocrCompPlatform_t *self, u64 value) {
    return 1;
}

u8 pthreadSendMessage(ocrCompPlatform_t *self, ocrPhysicalLocation_t target,
                      ocrPolicyMsg_t **message) {

    // TODO: Need to implement some sort of queues and then use signal/wait
    ASSERT(0);
    return 0;
}

u8 pthreadPollMessage(ocrCompPlatform_t *self, ocrPolicyMsg_t **message) {
    ASSERT(0);
    return 0;
}

u8 pthreadWaitMessage(ocrCompPlatform_t *self, ocrPolicyMsg_t **message) {
    ASSERT(0);
    return 0;
}

ocrCompPlatform_t* newCompPlatformPthread(ocrCompPlatformFactory_t *factory,
                                          ocrPhysicalLocation_t location,
                                          ocrParamList_t *perInstance) {

    pthread_once(&selfKeyInitialized,  initializeKey);
    ocrCompPlatformPthread_t * compPlatformPthread = (ocrComplPlatformPthread_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformPthread_t), NULL);

    compPlatformPthread->base.location = location;
    
    paramListCompPlatformPthread_t * params =
        (paramListCompPlatformPthread_t *) perInstance;
    if ((params != NULL) && (params->isMasterThread)) {
        // This particular instance is the master thread
        ocrCompPlatformFactoryPthread_t * pthreadFactory =
            (ocrCompPlatformFactoryPthread_t *) factory;
        compPlatformPthread->base.fctPtrs = &(pthreadFactory->masterPlatformFcts);
    } else {
        // This is a regular thread, get regular function pointers
        compPlatformPthread->base.fctPtrs = &(factory->platformFcts);
    }
    compPlatformPthread->binding = (params != NULL) ? params->binding : -1;
    compPlatformPthread->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;
    
    return (ocrCompPlatform_t*)compPlatformPthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

static ocrPolicyDomain_t * getCurrentPDPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->pd;
}

static void setCurrentPDPthread(ocrPolicyDomain_t *val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->pd = val;
}

void destructCompPlatformFactoryPthread(ocrCompPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

void setIdentifyingFunctionsPthread(ocrCompPlatformFactory_t *factory) {
    // FIXME.
    getCurrentPD = getCurrentPDPthread;
    setCurrentPD = setCurrentPDPthread;
    getMasterPD = getCurrentPDPthread;
    getCurrentEDT = getCurrentEDTFromWorker;
    setCurrentEDT = setCurrentEDTToWorker;
}

ocrCompPlatformFactory_t *newCompPlatformFactoryPthread(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformFactoryPthread_t), NULL);

    ocrCompPlatformFactoryPthread_t * derived = (ocrCompPlatformFactoryPthread_t *) base;

    base->instantiate = &newCompPlatformPthread;
    base->destruct = &destructCompPlatformFactoryPthread;
    base->setIdentifyingFunctions = &setIdentifyingFunctionsPthread;
    base->platformFcts.destruct = &pthreadDestruct;
    base->platformFcts.start = &pthreadStart;
    base->platformFcts.stop = &pthreadStop;
    base->platformFcts.finish = &pthreadFinish;
    base->platformFcts.getThrottle = &pthreadGetThrottle;
    base->platformFcts.setThrottle = &pthreadSetThrottle;
    base->platformFcts.sendMessage = &pthreadSendMessage;
    base->platformFcts.pollMessage = &pthreadPollMessage;
    base->platformFcts.waitMessage = &pthreadWaitMessage;

    // Setup master thread function pointer in the pthread factory
    derived->masterPlatformFcts.destruct = &pthreadDestruct;
    derived->masterPlatformFcts.finish = &pthreadFinish;
    derived->masterPlatformFcts.start = &pthreadStartMaster;
    derived->masterPlatformFcts.stop = &pthreadStopMaster;
    derived->masterPlatfromFcts.getThrottle = &pthreadGetThrottle;
    derived->masterPlatformFcts.setThrottle = &pthreadSetThrottle;
    derived->masterPlatfromFcts.sendMessage = &pthreadSendMessage;
    derived->masterPlatformFcts.pollMessage = &pthreadPollMessage;
    derived->masterPlatformFcts.waitMessage = &pthreadWaitMessage;

    paramListCompPlatformPthread_t * params =
      (paramListCompPlatformPthread_t *) perType;
    derived->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;

    return base;
}
#endif /* ENABLE_COMP_PLATFORM_PTHREAD */
