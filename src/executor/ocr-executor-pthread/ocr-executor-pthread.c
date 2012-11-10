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

#include "ocr-executor.h"

/******************************************************/
/* PTHREAD executor                                   */
/******************************************************/

typedef struct {
    ocr_executor_t executor;
    pthread_t os_thread;
    size_t stack_size;
} ocr_executor_pthread_t;

/**
 * The worker key we use to be able to find which worker
 * the executor thread is running
 */
pthread_key_t worker_key;
pthread_once_t worker_key_initialized = PTHREAD_ONCE_INIT;

/**
 * See ocr_executor_pthread_constructor
 */
void initialize_pthread_worker_key() {
    int rt = pthread_key_create(&worker_key, NULL);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_key_create\n"); exit(1); }
}

void ocr_executor_pthread_start(ocr_executor_t * executor) {
    ocr_executor_pthread_t * pthreadExecutor = (ocr_executor_pthread_t *) executor;
    int rt = 0;
    pthread_attr_t attr;
    rt = pthread_attr_init(&attr);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_attr_init\n"); exit(1); }
    //Note this call may fail if the system doesn't like the stack size asked for.
    rt = pthread_attr_setstacksize(&attr, pthreadExecutor->stack_size);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_attr_setstacksize\n"); exit(1); }
    rt = pthread_create(&(pthreadExecutor->os_thread), &attr, pthreadExecutor->executor.routine, pthreadExecutor->executor.routine_arg);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_create\n"); exit(1); }
}

void ocr_executor_pthread_stop(ocr_executor_t * executor) {
    ocr_executor_pthread_t * pthreadExecutor = (ocr_executor_pthread_t *) executor;
    int rt = pthread_join(pthreadExecutor->os_thread, NULL);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_join\n"); exit(1); }
}

void ocr_executor_pthread_create ( ocr_executor_t * base, void * configuration) {
    ocr_executor_pthread_t * derived = (ocr_executor_pthread_t *) base;
    if (configuration != NULL) {
        //TODO passed in configuration
        //derived->stack_size = stack_size;
    } else {
        derived->stack_size = 8388608;
    }
}

void ocr_executor_pthread_destruct (ocr_executor_t * base) {
    free(base);
}

ocr_executor_t * ocr_executor_pthread_constructor () {
    // This is it initialize the pthread-local storage that stores
    // the current worker the thread is executing.
    pthread_once(&worker_key_initialized, initialize_pthread_worker_key);
    ocr_executor_t * executor = (ocr_executor_t *) malloc(sizeof(ocr_executor_pthread_t));
    ocr_module_t * module_base = (ocr_module_t *) executor;
    module_base->map_fct = NULL;
    executor->routine = NULL;
    executor->routine_arg = NULL;
    executor->create = ocr_executor_pthread_create;
    executor->destruct = ocr_executor_pthread_destruct;
    executor->start = ocr_executor_pthread_start;
    executor->stop = ocr_executor_pthread_stop;
    return executor;
}

void associate_executor_and_worker(ocr_worker_t * worker) {
    int rt = pthread_setspecific(worker_key, worker);
    if (rt != 0) { printf("[PTHREAD] - ERROR in pthread_setspecific\n"); exit(1); }
}

ocrGuid_t ocr_get_current_worker_guid() {
    return get_worker_guid(((ocr_worker_t*)pthread_getspecific(worker_key)));
}
