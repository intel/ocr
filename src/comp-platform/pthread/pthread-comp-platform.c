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
#include <string.h>

#include "pthread-comp-platform.h"
#include "ocr-comp-platform.h"
#include "ocr-guid.h"
#include "ocr-policy-domain.h"

#include "debug.h"
#include "ocr-macros.h"

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
    free(base);
}

static void * pthreadRoutineWrapper(void * arg) {
  // Wrapper routine to allow initialization of local storage
  // before entering the worker routine.
  perThreadStorage_t *data = (perThreadStorage_t*)checkedMalloc(data, sizeof(perThreadStorage_t));
  RESULT_ASSERT(pthread_setspecific(selfKey, data), ==, 0);
  if (arg != NULL) {
    launchArg_t * launchArg = (launchArg_t *) arg;
    return launchArg->routine(arg);
  }
  return NULL;
}

static void pthreadStart(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;

    pthread_attr_t attr;
    RESULT_ASSERT(pthread_attr_init(&attr), ==, 0);
    //Note this call may fail if the system doesn't like the stack size asked for.
    RESULT_ASSERT(pthread_attr_setstacksize(&attr, pthreadCompPlatform->stackSize), ==, 0);
    RESULT_ASSERT(pthread_create(&(pthreadCompPlatform->osThread),
                                 &attr, &pthreadRoutineWrapper,
                                 launchArg), ==, 0);
}

static void pthreadStartMaster(ocrCompPlatform_t * compPlatform, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    //ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    // This comp-platform represent the currently executing master thread.
    // Pass NULL launchArgs so that the code sets the TLS but doesn't execute the worker routine.
    pthreadRoutineWrapper(NULL);
}

static void pthreadStop(ocrCompPlatform_t * compPlatform) {
    // This code must be called by thread '0' to join on other threads
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;
    RESULT_ASSERT(pthread_join(pthreadCompPlatform->osThread, NULL), ==, 0);
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
    RESULT_ASSERT(pthread_setspecific(selfKey, vals), ==, 0);
}

ocrPolicyDomain_t * getCurrentPDPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->pd;
}

static void setCurrentPDPthread(ocrPolicyDomain_t *val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->pd = val;
    RESULT_ASSERT(pthread_setspecific(selfKey, vals), ==, 0);
}

static void destructCompPlatformFactoryPthread(ocrCompPlatformFactory_t *factory) {
    free(factory);
}

ocrCompPlatformFactory_t *newCompPlatformFactoryPthread(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        checkedMalloc(base, sizeof(ocrCompPlatformFactoryPthread_t));

    ocrCompPlatformFactoryPthread_t * derived = (ocrCompPlatformFactoryPthread_t *) base;

    base->instantiate = &newCompPlatformPthread;
    base->destruct = &destructCompPlatformFactoryPthread;
    base->platformFcts.destruct = &pthreadDestruct;
    base->platformFcts.start = &pthreadStart;
    base->platformFcts.stop = &pthreadStop;
    getCurrentWorkerContext = &getCurrentWorkerContextPthread;
    setCurrentWorkerContext = &setCurrentWorkerContextPthread;
//    getCurrentPD = &getCurrentPDPthread;   FIXME: This needs to be moved elsewhere... start() maybe?
    setCurrentPD = &setCurrentPDPthread;

    // Setup master thread function pointer in the pthread factory
    memcpy(&(derived->masterPlatformFcts), &(base->platformFcts), sizeof(ocrCompPlatformFcts_t));
    derived->masterPlatformFcts.start = &pthreadStartMaster;

    paramListCompPlatformPthread_t * params = 
      (paramListCompPlatformPthread_t *) perType;
    derived->stackSize = ((params != NULL) && (params->stackSize > 0)) ? params->stackSize : 8388608;

    return base;
}
