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

#include "pthread-comp-platform.h"
#include "ocr-comp-platform.h"
#include "ocr-guid.h"

#include "debug.h"
#include "ocr-macros.h"


/**
 * @brief Structure stored on a per-thread basis to keep track of
 * "who we are"
 */
typedef struct {
    ocrGuid_t compTarget;
    ocrGuid_t edt;
    ocrGuid_t policyDomain;
} perThreadStorage_t;

/**
 * @brief Structure to launch the pthread
 *
 * This contains the subroutine (actual routine to launch) after initialization
 * has been done
 */
typedef struct {
    void (*subRoutine)(void*);
    void* arg;
} launchArg_t;

/**
 * The key we use to be able to find which compPlatform we are on
 */
pthread_key_t selfKey;
pthread_once_t selfKeyInitialized = PTHREAD_ONCE_INIT;

/**
 * @brief Routine that will be started in this pthread
 */
static void wrapperPthreadRun(void *arg) {
    launchArg_t *launchArgs = (launchArg_t *)arg;
    perThreadStorage_t *data = (perThreadStorage_t*)checkedMalloc(data, sizeof(perThreadStorage_t));
    RESULT_ASSERT(pthread_key_setspecific(selfKey, data), ==, 0);

    launchArgs->subRoutine(launchArgs->arg);

    free(launchArgs);
}


static void pthreadDestruct (ocrCompPlatform_t * base) {
    free(base);
}

static void pthreadStart(ocrCompPlatform_t * compPlatform) {
    ocrCompPlatformPthread_t * pthreadCompPlatform = (ocrCompPlatformPthread_t *) compPlatform;

    // Build the launch arguments
    launchArg_t *launchArgs = (launchArg_t*)checkedMalloc(launchArgs, sizeof(launchArg_t));
    launchArgs->subRoutine = pthreadCompPlatform->routine;
    launchArgs->arg = pthreadCompPlatform->arg;

    pthread_attr_t attr;
    RESULT_ASSERT(pthread_attr_init(&attr), ==, 0);
    //Note this call may fail if the system doesn't like the stack size asked for.
    RESULT_ASSERT(pthread_attr_setstacksize(&attr, pthreadCompPlatform->stackSize), ==, 0);
    RESULT_ASSERT(pthread_create(&(pthreadCompPlatform->osThread),
                                 &attr, &wrapperPthreadRun,
                                 launchArgs), ==, 0);
}

static void pthreadStop(ocrCompPlatform_t * compPlatform) {
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
        compPlatform, sizeof(ocrCompPlatformPthread_t));

    compPlatformPthread->base.module.mapFct = NULL;
    compPlatformPthread->base.fctPtrs = &(factory->platformFcts);

    // TODO: Extract information from perInstance. Will be in misc
    compPlatformPthread->routine = NULL;
    compPlatformPthread->routineArg = NULL;

    if (perInstance != NULL) {
        //TODO passed in configuration
        //derived->stackSize = stackSize;
    } else {
        compPlatformPthread->stackSize = 8388608;
    }

    return (ocrCompPlatform_t*)compPlatformPthread;
}

/******************************************************/
/* OCR COMP PLATFORM PTHREAD FACTORY                  */
/******************************************************/
static ocrGuid_t getCurrentComputePthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->compTarget;
}

static ocrGuid_t getCurrentEDTPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->edt;
}

static ocrGuid_t getCurrentPDPthread() {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    return vals->policyDomain;
}

static void setCurrentComputePthread(ocrGuid_t val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->compTarget = val;
    RESULT_ASSERT(pthread_setspecific(selfKey, vals), ==, 0);
}

static void setCurrentEDTPthread(ocrGuid_t val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->edt = val;
    RESULT_ASSERT(pthread_setspecific(selfKey, vals), ==, 0);
}

static void setCurrentPDPthread(ocrGuid_t val) {
    perThreadStorage_t *vals = pthread_getspecific(selfKey);
    vals->policyDomain = val;
    RESULT_ASSERT(pthread_setspecific(selfKey, vals), ==, 0);
}

static void destructCompPlatformPthread(ocrCompPlatformFactory_t *factory) {
    free(factory);
}

ocrCompPlatformFactory_t *newCompPlatformFactoryPthread(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        checkedMalloc(derived, sizeof(ocrCompPlatformFactoryPthread_t));

    base->instantiate = &newCompPlatformPthread;
    base->destruct = &destructCompPlatformPthread;
    base->platformFcts.destruct = &pthreadDestruct;
    base->platformFcts.start = &pthreadStart;
    base->platformFcts.stop = &pthreadStop;

    getCurrentCompTarget = &getCurrentComputePthread;
    setCurrentComptTarget = &setCurrentComputePthread;
    getCurrentEDT = &getCurrentEDTPthread;
    setCurrentEDT = &setCurrentEDTPthread;
    getCurrentPD = &getCurrentPDPthread;
    setCurrentPD = &setCurrentPDPthread;

    return base;
}
