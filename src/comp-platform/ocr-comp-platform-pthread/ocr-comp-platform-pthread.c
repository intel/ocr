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
#include <stdio.h>
#include <stdlib.h>

#include "ocr-macros.h"
#include "ocr-comp-platform.h"

/******************************************************/
/* PTHREAD comp-platform                              */
/******************************************************/

typedef struct {
    ocr_comp_platform_t base;
    pthread_t os_thread;
    u64 stack_size;
} ocr_comp_platform_pthread_t;

/**
 * The worker key we use to be able to find which worker
 * the compPlatform thread is running
 */
pthread_key_t worker_key;
pthread_once_t worker_key_initialized = PTHREAD_ONCE_INIT;

/**
 * See ocr_comp_platform_pthread_constructor
 */
void initialize_pthread_worker_key() {
    int rt = pthread_key_create(&worker_key, NULL);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_key_create\n"); exit(1); }
}

void ocr_comp_platform_pthread_start(ocr_comp_platform_t * compPlatform) {
    ocr_comp_platform_pthread_t * pthreadCompPlatform = (ocr_comp_platform_pthread_t *) compPlatform;
    int rt = 0;
    pthread_attr_t attr;
    rt = pthread_attr_init(&attr);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_attr_init\n"); exit(1); }
    //Note this call may fail if the system doesn't like the stack size asked for.
    rt = pthread_attr_setstacksize(&attr, pthreadCompPlatform->stack_size);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_attr_setstacksize\n"); exit(1); }
    rt = pthread_create(&(pthreadCompPlatform->os_thread), &attr, pthreadCompPlatform->base.routine, pthreadCompPlatform->base.routine_arg);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_create\n"); exit(1); }
}

void ocr_comp_platform_pthread_stop(ocr_comp_platform_t * compPlatform) {
    ocr_comp_platform_pthread_t * pthreadCompPlatform = (ocr_comp_platform_pthread_t *) compPlatform;
    int rt = pthread_join(pthreadCompPlatform->os_thread, NULL);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_join\n"); exit(1); }
}

void ocr_comp_platform_pthread_create ( ocr_comp_platform_t * base, void * configuration) {
    ocr_comp_platform_pthread_t * derived = (ocr_comp_platform_pthread_t *) base;
    if (configuration != NULL) {
        //TODO passed in configuration
        //derived->stack_size = stack_size;
    } else {
        derived->stack_size = 8388608;
    }
}

void ocr_comp_platform_pthread_destruct (ocr_comp_platform_t * base) {
    free(base);
}

ocr_comp_platform_t * ocr_comp_platform_pthread_constructor () {
    // This is it initialize the pthread-local storage that stores
    // the current worker the thread is executing.
    pthread_once(&worker_key_initialized, initialize_pthread_worker_key);
    ocr_comp_platform_t * compPlatform = checkedMalloc(compPlatform, sizeof(ocr_comp_platform_pthread_t));
    ocrMappable_t * module_base = (ocrMappable_t *) compPlatform;
    module_base->mapFct = NULL;
    compPlatform->routine = NULL;
    compPlatform->routine_arg = NULL;
    compPlatform->create = ocr_comp_platform_pthread_create;
    compPlatform->destruct = ocr_comp_platform_pthread_destruct;
    compPlatform->start = ocr_comp_platform_pthread_start;
    compPlatform->stop = ocr_comp_platform_pthread_stop;
    return compPlatform;
}

void associate_comp_platform_and_worker(ocrWorker_t * worker) {
    int rt = pthread_setspecific(worker_key, worker);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_setspecific\n"); exit(1); }
}

ocrGuid_t ocr_get_current_worker_guid() {
    return get_worker_guid(((ocrWorker_t*)pthread_getspecific(worker_key)));
}
