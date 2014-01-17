/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_PTHREAD

#include "debug.h"

#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

#include "pthread-comp-platform.h"

#include <stdlib.h> // For malloc for TLS

#define DEBUG_TYPE COMP_PLATFORM

extern void bindThread(u32 mask);

/**
 * @brief Structure stored on a per-thread basis to keep track of
 * "who we are"
 */
typedef struct {
    ocrPolicyDomain_t *pd;
    ocrWorker_t *worker;
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
    // Only called on slave workers (never master)
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) arg;
    s32 cpuBind = pthreadCompPlatform->binding;
    if(cpuBind != -1) {
        DPRINTF(DEBUG_LVL_INFO, "Binding comp-platform to cpu_id %d\n", cpuBind);
        bindThread(cpuBind);
    }
    launchArg_t * launchArg = (launchArg_t *) pthreadCompPlatform->launchArg;
    // Wrapper routine to allow initialization of local storage
    // before entering the worker routine.
    perThreadStorage_t *data = (perThreadStorage_t*)malloc(sizeof(perThreadStorage_t));
    RESULT_ASSERT(pthread_setspecific(selfKey, data), ==, 0);
    ASSERT(launchArg);
    return pthreadRoutineExecute(launchArg);
}

static void destroyKey(void* arg) {
    runtimeChunkFree((u64)arg, NULL);
}

static void initializeKey() {
    RESULT_ASSERT(pthread_key_create(&selfKey, &destroyKey), ==, 0);
    // We are going to set our own key (we are the master thread)
    perThreadStorage_t *data = (perThreadStorage_t*)malloc(sizeof(perThreadStorage_t));
    RESULT_ASSERT(pthread_setspecific(selfKey, data), ==, 0);
}

void pthreadDestruct (ocrCompPlatform_t * base) {
    runtimeChunkFree((u64)base, NULL);
}

void pthreadBegin(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType) {
    
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    compPlatform->pd = PD;
    pthreadCompPlatform->isMaster = (workerType == MASTER_WORKERTYPE);
    if(pthreadCompPlatform->isMaster) {
        // Only do the binding
        s32 cpuBind = pthreadCompPlatform->binding;
        if(cpuBind != -1) {
            DPRINTF(DEBUG_LVL_INFO, "Binding comp-platform to cpu_id %d\n", cpuBind);
            bindThread(cpuBind);
        }
        // The master starts executing when we call "stop" on it
    }
}

void pthreadStart(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, ocrWorkerType_t workerType,
                  launchArg_t * launchArg) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    pthreadCompPlatform->launchArg = launchArg;
    if(!pthreadCompPlatform->isMaster) {
        pthread_attr_t attr;
        RESULT_ASSERT(pthread_attr_init(&attr), ==, 0);
        //Note this call may fail if the system doesn't like the stack size asked for.
        RESULT_ASSERT(pthread_attr_setstacksize(&attr, pthreadCompPlatform->stackSize), ==, 0);
        RESULT_ASSERT(pthread_create(&(pthreadCompPlatform->osThread),
                                     &attr, &pthreadRoutineWrapper,
                                     pthreadCompPlatform), ==, 0);
    } else {
        // The upper level worker will only start us once
        pthreadRoutineExecute(pthreadCompPlatform->launchArg);
    }
}

void pthreadStop(ocrCompPlatform_t * compPlatform) {
    // Nothing to do really
}

void pthreadFinish(ocrCompPlatform_t *compPlatform) {
    // This code will be called by the main thread to join everyone else
    ocrCompPlatformPthread_t *pthreadCompPlatform = (ocrCompPlatformPthread_t*)compPlatform;
    if(!pthreadCompPlatform->isMaster) {
        RESULT_ASSERT(pthread_join(pthreadCompPlatform->osThread, NULL), ==, 0);
    }
}

u8 pthreadGetThrottle(ocrCompPlatform_t *self, u64* value) {
    return 1;
}

u8 pthreadSetThrottle(ocrCompPlatform_t *self, u64 value) {
    return 1;
}

u8 pthreadSendMessage(ocrCompPlatform_t *self, ocrLocation_t target,
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

u8 pthreadSetCurrentEnv(ocrCompPlatform_t *self, ocrPolicyDomain_t *pd,
                        ocrWorker_t *worker) {

    ASSERT(pd->fguid.guid == self->pd->fguid.guid);
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->pd = pd;
    vals->worker = worker;
    return 0;
}

ocrCompPlatform_t* newCompPlatformPthread(ocrCompPlatformFactory_t *factory,
                                          ocrLocation_t location, ocrParamList_t *perInstance) {

    pthread_once(&selfKeyInitialized,  initializeKey);
    ocrCompPlatformPthread_t * compPlatformPthread = (ocrCompPlatformPthread_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformPthread_t), NULL);

    compPlatformPthread->base.location = location;
        
    paramListCompPlatformPthread_t * params =
        (paramListCompPlatformPthread_t *) perInstance;
    
    compPlatformPthread->base.fcts = factory->platformFcts;
    compPlatformPthread->binding = (params != NULL) ? params->binding : -1;
    compPlatformPthread->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;
    compPlatformPthread->isMaster = false;
    
    return (ocrCompPlatform_t*)compPlatformPthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

void destructCompPlatformFactoryPthread(ocrCompPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

void pthreadGetCurrentEnv(ocrPolicyDomain_t** pd, ocrWorker_t** worker,
                          ocrTask_t **task, ocrPolicyMsg_t* msg) {

    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    if(pd)
        *pd = vals->pd;
    if(worker)
        *worker = vals->worker;
    if(vals->worker && task)
        *task = vals->worker->curTask;
    if(vals->pd && msg)
        msg->srcLocation = vals->pd->myLocation;
}

void setGetCurrentEnvPthread(ocrCompPlatformFactory_t *factory) {
    getCurrentEnv = &pthreadGetCurrentEnv;
}

ocrCompPlatformFactory_t *newCompPlatformFactoryPthread(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformFactoryPthread_t), (void *)1);

    ocrCompPlatformFactoryPthread_t * derived = (ocrCompPlatformFactoryPthread_t *) base;

    base->instantiate = &newCompPlatformPthread;
    base->destruct = &destructCompPlatformFactoryPthread;
    base->setGetCurrentEnv = &setGetCurrentEnvPthread;
    base->platformFcts.destruct = &pthreadDestruct;
    base->platformFcts.begin = &pthreadBegin;
    base->platformFcts.start = &pthreadStart;
    base->platformFcts.stop = &pthreadStop;
    base->platformFcts.finish = &pthreadFinish;
    base->platformFcts.getThrottle = &pthreadGetThrottle;
    base->platformFcts.setThrottle = &pthreadSetThrottle;
    base->platformFcts.sendMessage = &pthreadSendMessage;
    base->platformFcts.pollMessage = &pthreadPollMessage;
    base->platformFcts.waitMessage = &pthreadWaitMessage;
    base->platformFcts.setCurrentEnv = &pthreadSetCurrentEnv;

    paramListCompPlatformPthread_t * params =
      (paramListCompPlatformPthread_t *) perType;
    derived->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;

    return base;
}
#endif /* ENABLE_COMP_PLATFORM_PTHREAD */
