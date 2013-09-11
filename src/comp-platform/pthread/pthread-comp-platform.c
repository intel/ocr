/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-worker.h"
#include "pthread-comp-platform.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Structure stored on a per-thread basis to keep track of
 * "who we are"
 */
typedef struct {
    ocrGuid_t compTarget;
    ocrPolicyCtx_t * ctx;
    ocrPolicyDomain_t * pd;
} perThreadStorage_t;

/**
 * The key we use to be able to find which compPlatform we are on
 */
pthread_key_t selfKey;
pthread_once_t selfKeyInitialized = PTHREAD_ONCE_INIT;

static void pthreadDestruct (ocrCompPlatform_t * base) {
    free(((ocrCompPlatformPthread_t*)base)->launchArg);
    free(base);
}

static void * pthreadRoutineExecute(launchArg_t * launchArg) {
  return launchArg->routine(launchArg);
}

static void * pthreadRoutineWrapper(void * arg) {
  ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) arg;
  launchArg_t * launchArg = (launchArg_t *) pthreadCompPlatform->launchArg;
  // Wrapper routine to allow initialization of local storage
  // before entering the worker routine.
  perThreadStorage_t *data = (perThreadStorage_t*)checkedMalloc(data, sizeof(perThreadStorage_t));
  RESULT_ASSERT(pthread_setspecific(selfKey, data), ==, 0);
  if (launchArg != NULL) {
    return pthreadRoutineExecute(launchArg);
  }
  return NULL;
}

static void pthreadStart(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    pthreadCompPlatform->launchArg = launchArg;
    pthread_attr_t attr;
    RESULT_ASSERT(pthread_attr_init(&attr), ==, 0);
    //Note this call may fail if the system doesn't like the stack size asked for.
    RESULT_ASSERT(pthread_attr_setstacksize(&attr, pthreadCompPlatform->stackSize), ==, 0);
    RESULT_ASSERT(pthread_create(&(pthreadCompPlatform->osThread),
                                 &attr, &pthreadRoutineWrapper,
                                 pthreadCompPlatform), ==, 0);
}

static void pthreadStop(ocrCompPlatform_t * compPlatform) {
    // This code must be called by thread '0' to join on other threads
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    RESULT_ASSERT(pthread_join(pthreadCompPlatform->osThread, NULL), ==, 0);
}

static void pthreadStartMaster(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    // This comp-platform represent the currently executing master thread.
    // Pass NULL launchArgs so that the code sets the TLS but doesn't execute the worker routine.
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    pthreadCompPlatform->launchArg = NULL;
    pthreadRoutineWrapper(pthreadCompPlatform);
    pthreadCompPlatform->launchArg = launchArg;
}

static void pthreadStopMaster(ocrCompPlatform_t * compPlatform) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    pthreadRoutineExecute(pthreadCompPlatform->launchArg);
}

static void destroyKey(void* arg) {
    free(arg);
}

static void initializeKey() {
    RESULT_ASSERT(pthread_key_create(&selfKey, &destroyKey), ==, 0);
}


static ocrCompPlatform_t* newCompPlatformPthread(ocrCompPlatformFactory_t *factory,
                                                 ocrParamList_t *perInstance) {

    pthread_once(&selfKeyInitialized,  initializeKey);
    ocrCompPlatformPthread_t * compPlatformPthread = checkedMalloc(
        compPlatformPthread, sizeof(ocrCompPlatformPthread_t));

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
    compPlatformPthread->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;
    compPlatformPthread->base.module.mapFct = NULL;
    return (ocrCompPlatform_t*)compPlatformPthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/

static ocrPolicyCtx_t * getCurrentWorkerContextPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->ctx;
}

static void setCurrentWorkerContextPthread(ocrPolicyCtx_t *val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->ctx = val;
}

static ocrPolicyDomain_t * getCurrentPDPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->pd;
}

static void setCurrentPDPthread(ocrPolicyDomain_t *val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->pd = val;
}

static void destructCompPlatformFactoryPthread(ocrCompPlatformFactory_t *factory) {
    free(factory);
}

static void setIdentifyingFunctionsPthread(ocrCompPlatformFactory_t *factory) {
    getCurrentPD = getCurrentPDPthread;
    setCurrentPD = setCurrentPDPthread;
    getMasterPD = getCurrentPDPthread;
    getCurrentWorkerContext = getCurrentWorkerContextPthread;
    setCurrentWorkerContext = setCurrentWorkerContextPthread;
    getCurrentEDT = getCurrentEDTFromWorker;
    setCurrentEDT = setCurrentEDTToWorker;
}

ocrCompPlatformFactory_t *newCompPlatformFactoryPthread(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        checkedMalloc(base, sizeof(ocrCompPlatformFactoryPthread_t));

    ocrCompPlatformFactoryPthread_t * derived = (ocrCompPlatformFactoryPthread_t *) base;

    base->instantiate = &newCompPlatformPthread;
    base->destruct = &destructCompPlatformFactoryPthread;
    base->setIdentifyingFunctions = &setIdentifyingFunctionsPthread;
    base->platformFcts.destruct = &pthreadDestruct;
    base->platformFcts.start = &pthreadStart;
    base->platformFcts.stop = &pthreadStop;

    // Setup master thread function pointer in the pthread factory
    memcpy(&(derived->masterPlatformFcts), &(base->platformFcts), sizeof(ocrCompPlatformFcts_t));
    derived->masterPlatformFcts.start = &pthreadStartMaster;
    derived->masterPlatformFcts.stop = &pthreadStopMaster;

    paramListCompPlatformPthread_t * params =
      (paramListCompPlatformPthread_t *) perType;
    derived->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;

    return base;
}
