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
    pthreadCompPlatform->launchArg = launchArg;
    // The master starts executing when we call "stop" on it
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
    ocrCompPlatformPthread_t * compPlatformPthread = (ocrCompPlatformPthread_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformPthread_t), NULL);

    compPlatformPthread->base.location = location;
    
    paramListCompPlatformPthread_t * params =
        (paramListCompPlatformPthread_t *) perInstance;
    if ((params != NULL) && (params->isMasterThread)) {
        // This particular instance is the master thread
        ocrCompPlatformFactoryPthread_t * pthreadFactory =
            (ocrCompPlatformFactoryPthread_t *) factory;
        compPlatformPthread->base.fcts = pthreadFactory->masterPlatformFcts;
    } else {
        // This is a regular thread, get regular function pointers
        compPlatformPthread->base.fcts = factory->platformFcts;
    }
    compPlatformPthread->binding = (params != NULL) ? params->binding : -1;
    compPlatformPthread->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;
    
    return (ocrCompPlatform_t*)compPlatformPthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

/*
 * Figure out where to put these 
static ocrPolicyDomain_t * getCurrentPDPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->pd;
}

static void setCurrentPDPthread(ocrPolicyDomain_t *val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->pd = val;
}
*/

void destructCompPlatformFactoryPthread(ocrCompPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrCompPlatformFactory_t *newCompPlatformFactoryPthread(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformFactoryPthread_t), NULL);

    ocrCompPlatformFactoryPthread_t * derived = (ocrCompPlatformFactoryPthread_t *) base;

    base->instantiate = &newCompPlatformPthread;
    base->destruct = &destructCompPlatformFactoryPthread;
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
    derived->masterPlatformFcts.getThrottle = &pthreadGetThrottle;
    derived->masterPlatformFcts.setThrottle = &pthreadSetThrottle;
    derived->masterPlatformFcts.sendMessage = &pthreadSendMessage;
    derived->masterPlatformFcts.pollMessage = &pthreadPollMessage;
    derived->masterPlatformFcts.waitMessage = &pthreadWaitMessage;

    paramListCompPlatformPthread_t * params =
      (paramListCompPlatformPthread_t *) perType;
    derived->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;

    return base;
}
#endif /* ENABLE_COMP_PLATFORM_PTHREAD */
