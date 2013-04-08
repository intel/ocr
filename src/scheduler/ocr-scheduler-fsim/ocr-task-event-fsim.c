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

#include <stdlib.h>
#include <assert.h>

#include "fsim.h"
#include "ocr-datablock.h"
#include "ocr-utils.h"
#include "debug.h"

int fsim_task_is_message ( fsim_message_interface_t* fsim_base ) {
    return 0;
}

void fsim_task_destruct ( ocr_task_t* base ) {
    fsim_task_t* derived = (fsim_task_t*) base;
    free(derived);
}

fsim_task_t* fsim_task_construct (ocrEdt_t funcPtr, u32 paramc, u64 * params, void** paramv, size_t dep_list_size) {
    fsim_task_t* derived = (fsim_task_t*) malloc(sizeof(fsim_task_t));
    hc_task_t* hcTaskBase = &(derived->fsimBase.base);

    hcTaskBase->awaitList = hc_await_list_constructor(dep_list_size);
    hc_task_construct_internal(hcTaskBase, funcPtr, paramc, params, paramv);

    fsim_message_interface_t* fsimMessage = &(derived->fsimBase.message_interface);
    fsimMessage->is_message = fsim_task_is_message;

    ocr_task_t* ocrTaskBase = (ocr_task_t*) hcTaskBase;
    ocrTaskBase->destruct = fsim_task_destruct;
    return derived;
}

void fsim_task_factory_destructor ( struct ocr_task_factory_struct* base ) {
    fsim_task_factory* derived = (fsim_task_factory*) base;
    free(derived);
}

ocrGuid_t fsim_task_factory_create ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, size_t dep_l_size) {
    fsim_task_t* edt = fsim_task_construct(fctPtr, paramc, params, paramv, dep_l_size);
    ocr_task_t* base = (ocr_task_t*) edt;
    return base->guid;
}

struct ocr_task_factory_struct* fsim_task_factory_constructor(void) {
    fsim_task_factory* derived = (fsim_task_factory*) malloc(sizeof(fsim_task_factory));
    ocr_task_factory* base = (ocr_task_factory*) derived;
    base->create = fsim_task_factory_create;
    base->destruct =  fsim_task_factory_destructor;
    return base;
}

void fsim_message_task_factory_destructor ( struct ocr_task_factory_struct* base ) {
    fsim_message_task_factory* derived = (fsim_message_task_factory*) base;
    free(derived);
}

int fsim_message_task_is_message ( fsim_message_interface_t* fsim_base ) {
    return 1;
}

void fsim_message_task_destruct ( ocr_task_t* base ) {
    fsim_task_t* derived = (fsim_task_t*) base;
    free(derived);
}

fsim_message_task_t* fsim_message_task_construct (ocrEdt_t funcPtr) {
    fsim_message_task_t* derived = (fsim_message_task_t*) malloc(sizeof(fsim_message_task_t));
    hc_task_t* hcTaskBase = &(derived->fsimBase.base);

    hcTaskBase->awaitList = NULL;
    hc_task_construct_internal(hcTaskBase, NULL, 0, NULL, NULL);

    fsim_message_interface_t* fsimMessage = &(derived->fsimBase.message_interface);
    fsimMessage->is_message = fsim_message_task_is_message;

    ocr_task_t* ocrTaskBase = (ocr_task_t*) hcTaskBase;
    ocrTaskBase->destruct = fsim_message_task_destruct;

    return derived;
}

ocrGuid_t fsim_message_task_factory_create ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, u32 paramc, u64 * params, void** paramv, size_t dep_l_size) {
    fsim_message_task_t* edt = fsim_message_task_construct(fctPtr);
    ocr_task_t* base = (ocr_task_t*) edt;
    return base->guid;
}

struct ocr_task_factory_struct* fsim_message_task_factory_constructor(void) {
    fsim_message_task_factory* derived = (fsim_message_task_factory*) malloc(sizeof(fsim_message_task_factory));
    ocr_task_factory* base = (ocr_task_factory*) derived;
    base->create = fsim_message_task_factory_create;
    base->destruct =  fsim_message_task_factory_destructor;
    return base;
}

void * xe_worker_computation_routine (void * arg) {
    ocr_worker_t * baseWorker = (ocr_worker_t *) arg;

    /* associate current thread with the worker */
    associate_executor_and_worker(baseWorker);

    ocrGuid_t workerGuid = get_worker_guid(baseWorker);
    ocr_scheduler_t * xeScheduler = get_worker_scheduler(baseWorker);

    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(baseWorker));

    while ( baseWorker->is_running(baseWorker) ) {
        //
        // try to extract work from an XE's workpile of assigned-by-CE-tasks
        // as of now always of size 1
        ocrGuid_t taskGuid = xeScheduler->take(xeScheduler, workerGuid);

        if ( NULL_GUID != taskGuid ) {
            // if managed to find work that was assigned by CE, execute it
            ocr_task_t* currTask = NULL;
            globalGuidProvider->getVal(globalGuidProvider, taskGuid, (u64*)&(currTask), NULL);
            baseWorker->setCurrentEDT(baseWorker,taskGuid);
            currTask->execute(currTask);
            baseWorker->setCurrentEDT(baseWorker, NULL_GUID);
        } else {
            // TODO sagnak, this assumes (*A LOT*) the structure below, is this fair?
            // no assigned work found, now we have to create a 'message task'
            // by using our policy domain's message task factory
            ocr_policy_domain_t* policy_domain = xeScheduler->domain;
            ocr_task_factory* message_task_factory = policy_domain->taskFactories[1];

            // the message to the CE says 'give me work' and notes who is asking for it
            ocrGuid_t messageTaskGuid = message_task_factory->create(message_task_factory, NULL, 0, NULL, NULL, 0);
            fsim_message_task_t* derived = NULL;
            globalGuidProvider->getVal(globalGuidProvider, messageTaskGuid, (u64*)&(derived), NULL);
            derived -> type = GIVE_ME_WORK;
            derived -> from_worker_guid = workerGuid;

            // there is no work left and the message has been sent, so turn of the worker
            // baseWorker->stop(baseWorker);
            xe_worker_t* derivedWorker = (xe_worker_t*)baseWorker;
            pthread_mutex_lock(&derivedWorker->isRunningMutex);
            if ( baseWorker->is_running(baseWorker) ) {
                // give the work to the XE scheduler, which in turn should give it to the CE
                // through policy domain hand out and the scheduler differentiates tasks by type (RTTI) like
                xeScheduler->give(xeScheduler, workerGuid, messageTaskGuid);
                pthread_cond_wait(&derivedWorker->isRunningCond, &derivedWorker->isRunningMutex);
            }
            pthread_mutex_unlock(&derivedWorker->isRunningMutex);
        }
    }
    return NULL;
}

void * ce_worker_computation_routine(void * arg) {
    ocr_worker_t * ceWorker = (ocr_worker_t *) arg;
    ocr_scheduler_t * ceScheduler = get_worker_scheduler(ceWorker);

    /* associate current thread with the worker */
    associate_executor_and_worker(ceWorker);

    ocrGuid_t ceWorkerGuid = get_worker_guid(ceWorker);
    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(ceWorker));
    while(ceWorker->is_running(ceWorker)) {

        // pop a 'message task' to handle messaging
        // the 'state' of the scheduler should point to the message workpile
        ocrGuid_t messageTaskGuid = ceScheduler->take(ceScheduler, ceWorkerGuid);

        // TODO sagnak, oh the assumptions, clean below
        if ( NULL_GUID != messageTaskGuid ) {

            // check if the popped task is of 'message task' type
            fsim_task_base_t* currTask = NULL;
            globalGuidProvider->getVal(globalGuidProvider, messageTaskGuid, (u64*)&(currTask), NULL);

            fsim_message_interface_t* taskAsMessage = &(currTask->message_interface);
            assert(taskAsMessage->is_message(taskAsMessage) && "hoping to pop a message-task; popped non-message task");

            // TODO sagnak, we seem to 'know' the underlying task type, not sure if this is a good idea
            fsim_message_task_t* currMessageTask = (fsim_message_task_t*) currTask;

            // TODO sagnak, we seem to 'know' the underlying scheduler type, not sure if this is a good idea
            ce_scheduler_t* derivedCeScheduler = (ce_scheduler_t*) ceScheduler;

            // find the worker, scheduler the message originated from
            ocrGuid_t targetWorkerGuid = currMessageTask->from_worker_guid;
            ocr_worker_t* targetWorker= NULL;
            globalGuidProvider->getVal(globalGuidProvider, targetWorkerGuid, (u64*)&(targetWorker), NULL);
            ocr_scheduler_t* targetScheduler = get_worker_scheduler(targetWorker);

            switch (currMessageTask->type) {
                case GIVE_ME_WORK:
                    {
                        // if the XE is asking for work
                        // change the scheduler 'state' to 'executable task' popping
                        derivedCeScheduler -> in_message_popping_mode = 0;
                        // pop an 'executable task'
                        ocrGuid_t toXeTaskGuid = ceScheduler->take(ceScheduler, ceWorkerGuid);
                        // change the scheduler 'state' back to 'message task' popping
                        derivedCeScheduler -> in_message_popping_mode = 1;

                        if ( NULL_GUID != toXeTaskGuid ) {
                            targetScheduler->give(targetScheduler, ceWorkerGuid, toXeTaskGuid);
                            // now that the XE has work, it may be restarted
                            // targetWorker->start(targetWorker);
                            //
                            xe_worker_t* derivedWorker = (xe_worker_t*)targetWorker;
                            pthread_mutex_lock(&derivedWorker->isRunningMutex);
                            pthread_cond_signal(&derivedWorker->isRunningCond);
                            pthread_mutex_unlock(&derivedWorker->isRunningMutex);
                        } else {
                            // if there was no successful task handing to XE, do not lose the message
                            ceScheduler->give(ceScheduler, ceWorkerGuid, messageTaskGuid);
                        }
                    }
                    break;
                case PICK_MY_WORK_UP:
                    {
                        ocrGuid_t taskFromXe = targetScheduler->take( targetScheduler, ceWorkerGuid );
                        if ( NULL_GUID != taskFromXe ) {
                            ceScheduler->give(ceScheduler, ceWorkerGuid, taskFromXe);
                        } else {
                            // if there was no successful task picking from XE, do not lose the message
                            ceScheduler->give(ceScheduler, ceWorkerGuid, messageTaskGuid);
                        }
                    }
                    break;
            };

            // ceWorker->setCurrentEDT(ceWorker,messageTaskGuid);
            // curr_task->execute(curr_task);
            // ceWorker->setCurrentEDT(ceWorker, NULL_GUID);
        }
    }
    return NULL;
}
