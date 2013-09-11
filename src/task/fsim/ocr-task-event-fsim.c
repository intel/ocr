/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdlib.h>
#include <assert.h>

#include "fsim.h"
#include "ocr-datablock.h"
#include "ocr-utils.h"
#include "debug.h"
#include "ocr-macros.h"

#define END_OF_LIST NULL

// This is doing pretty much what the hc implementation would do
// There's no inheritance setup so it's a vague copy paste
static void newTaskFsimInternal (ocrPolicyDomain_t * pd, ocrTaskHc_t* derived,
        ocrTaskTemplate_t * taskTemplate, u64 * params, void** paramv, ocrGuid_t outputEvent) {
    u32 nbDeps = taskTemplate->depc;
    if (nbDeps == 0) {
        derived->signalers = END_OF_LIST;
    } else {
        // Since we know how many dependences we have, preallocate signalers
        derived->signalers = checkedMalloc(derived->signalers, sizeof(regNode_t)*nbDeps);
    }
    derived->waiters = END_OF_LIST;
    // Initialize base
    ocrTask_t* base = (ocrTask_t*) derived;
    base->guid = UNINITIALIZED_GUID;
    guidify(pd, (u64)base, &(base->guid), OCR_GUID_EDT);
    if (taskTemplate != NULL) {
        base->templateGuid = taskTemplate->guid;
    }
    base->params = params;
    base->paramv = paramv;
    base->outputEvent = outputEvent;
    // Initialize ELS
    int i = 0;
    while (i < ELS_SIZE) {
        base->els[i++] = NULL_GUID;
    }
}

int fsim_task_is_message ( fsim_message_interface_t* fsim_base ) {
    return 0;
}

void destructTaskFsim ( ocrTask_t* base ) {
    ocrTaskFsim_t* derived = (ocrTaskFsim_t*) base;
    free(derived);
}

void destructTaskFactoryFsim ( ocrTaskFactory_t* base ) {
    ocrTaskFactoryFsim_t* derived = (ocrTaskFactoryFsim_t*) base;
    free(derived);
}


/******************************************************/
/* Fsim Task Template Factory                         */
/******************************************************/

ocrTaskTemplate_t * newTaskTemplateFsim (ocrTaskTemplateFactory_t* factory, ocrEdt_t executePtr, u32 paramc, u32 depc) {
    ocrTaskTemplateHc_t* template = (ocrTaskTemplateHc_t*) checkedMalloc(template, sizeof(ocrTaskTemplateHc_t));
    ocrTaskTemplate_t * base = (ocrTaskTemplate_t *) template;
    base->paramc = paramc;
    base->depc = depc;
    base->executePtr = executePtr;
    return base;
}

void destructTaskTemplateFactoryFsim(ocrTaskTemplateFactory_t* base) {
    ocrTaskTemplateFactoryHc_t* derived = (ocrTaskTemplateFactoryHc_t*) base;
    free(derived);
}

ocrTaskTemplateFactory_t * newTaskTemplateFactoryFsim(ocrParamList_t* perType) {
    ocrTaskTemplateFactoryHc_t* derived = (ocrTaskTemplateFactoryHc_t*) checkedMalloc(derived, sizeof(ocrTaskTemplateFactoryHc_t));
    ocrTaskTemplateFactory_t* base = (ocrTaskTemplateFactory_t*) derived;
    base->instantiate = newTaskTemplateFsim;
    base->destruct =  destructTaskTemplateFactoryFsim;
    //TODO What taskTemplateFcts is supposed to do ?
    return base;
}

ocrTask_t * newTaskFsim ( ocrTaskFactory_t* factory, ocrTaskTemplate_t * taskTemplate, u64 * params, void** paramv, u16 properties, ocrGuid_t * outputEventPtr) {
    ocrTaskFsim_t* derived = (ocrTaskFsim_t*) malloc(sizeof(ocrTaskFsim_t));
    ocrTaskHc_t* hcBase = &(derived->fsimBase.base);
    ocrTask_t* base = &(hcBase->base);
    ocrPolicyDomain_t* pd = getCurrentPD();
    if (outputEventPtr != NULL) {
        ASSERT(false && "fsim implementation doesn't support output event");
    }
    newTaskFsimInternal(pd, hcBase, taskTemplate, params, paramv, NULL_GUID);
    derived->fsimBase.message_interface.is_message = fsim_task_is_message;
    base->fctPtrs = &(((ocrTaskFactoryHc_t *)factory)->taskFctPtrs);
    return base;
}

ocrTaskFactory_t* newTaskFactoryFsim(ocrParamList_t* perInstance) {
    ocrTaskFactoryFsim_t* derived = (ocrTaskFactoryFsim_t*) malloc(sizeof(ocrTaskFactoryFsim_t));
    ocrTaskFactoryHc_t* baseHc = (ocrTaskFactoryHc_t*) derived;
    ocrTaskFactory_t* base = (ocrTaskFactory_t*) derived;
    base->instantiate = newTaskFsim;
    base->destruct =  destructTaskFactoryFsim;
    // initialize singleton instance that carries hc implementation
    // function pointers. Every instantiated task template will use
    // this pointer to resolve functions implementations.
    baseHc->taskFctPtrs.destruct = destructTaskFsim;
    baseHc->taskFctPtrs.execute = taskExecute;
    baseHc->taskFctPtrs.schedule = tryScheduleTask;
    return base;
}

void destructTaskFactoryFsimMessage ( ocrTaskFactory_t* base ) {
    ocrTaskFactoryFsimMessage_t* derived = (ocrTaskFactoryFsimMessage_t*) base;
    free(derived);
}

int fsim_message_task_is_message ( fsim_message_interface_t* fsim_base ) {
    return 1;
}

void destructTaskFsimMessage ( ocrTask_t* base ) {
    ocrTaskFsim_t* derived = (ocrTaskFsim_t*) base;
    free(derived);
}

ocrTask_t * newTaskFsimMessage(ocrTaskFactory_t* factory, ocrTaskTemplate_t * taskTemplate, u64 * params, void** paramv, u16 properties, ocrGuid_t * outputEventPtr) {
    ocrTaskFsimMessage_t* derived = (ocrTaskFsimMessage_t*) malloc(sizeof(ocrTaskFsimMessage_t));
    ocrTaskHc_t* hcBase = &(derived->fsimBase.base);
    ocrTask_t* base = &(hcBase->base);
    ocrPolicyDomain_t* pd = getCurrentPD();
    if (outputEventPtr != NULL) {
        ASSERT(false && "fsim implementation doesn't support output event");
    }
    // This is an internal task used as a "message", we shouldn't need to setup a task template
    newTaskFsimInternal(pd, hcBase, NULL, NULL, NULL, NULL_GUID);
    derived->fsimBase.message_interface.is_message = fsim_message_task_is_message;
    base->fctPtrs = &(((ocrTaskFactoryHc_t *)factory)->taskFctPtrs);
    return (ocrTask_t*) derived;
}

ocrTaskFactory_t* newTaskFactoryFsimMessage(ocrParamList_t* perInstance) {
    ocrTaskFactoryFsimMessage_t* derived = (ocrTaskFactoryFsimMessage_t*) malloc(sizeof(ocrTaskFactoryFsimMessage_t));
    ocrTaskFactoryHc_t* baseHc = (ocrTaskFactoryHc_t*) derived;
    ocrTaskFactory_t* base = (ocrTaskFactory_t*) derived;
    base->instantiate = newTaskFsimMessage;
    base->destruct =  destructTaskFactoryFsimMessage;

    // initialize singleton instance that carries implementation function pointers
    baseHc->taskFctPtrs.destruct = destructTaskFsimMessage;
    baseHc->taskFctPtrs.execute = taskExecute;
    baseHc->taskFctPtrs.schedule = tryScheduleTask;
    return base;
}

void * xe_worker_computation_routine (void * arg) {
    ocrWorker_t * baseWorker = (ocrWorker_t *) arg;

    /* associate current thread with the worker */
    associate_comp_platform_and_worker(baseWorker);

    ocrGuid_t workerGuid = get_worker_guid(baseWorker);
    ocrScheduler_t * xeScheduler = get_worker_scheduler(baseWorker);

    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(baseWorker));

    while ( baseWorker->fctPtrs->isRunning(baseWorker) ) {
        //
        // try to extract work from an XE's workpile of assigned-by-CE-tasks
        // as of now always of size 1
        ocrGuid_t taskGuid = xeScheduler->take(xeScheduler, workerGuid);

        if ( NULL_GUID != taskGuid ) {
            // if managed to find work that was assigned by CE, execute it
            ocrTask_t* currTask = NULL;
            deguidify(getCurrentPD(), taskGuid, (u64*)&(currTask), NULL);
            baseWorker->fctPtrs->setCurrentEDT(baseWorker,taskGuid);
            currTask->fctPtrs->execute(currTask);
            baseWorker->fctPtrs->setCurrentEDT(baseWorker, NULL_GUID);
        } else {
            // TODO sagnak, this assumes (*A LOT*) the structure below, is this fair?
            // no assigned work found, now we have to create a 'message task'
            // by using our policy domain's message task factory
            ocrPolicyDomainFsim_t* policy_domain = (ocrPolicyDomainFsim_t *) xeScheduler->domain;
            ocrTaskFactory_t* message_task_factory = policy_domain->taskMessageFactory;
            // the message to the CE says 'give me work' and notes who is asking for it
            ocrTaskFsimMessage_t * derived = (ocrTaskFsimMessage_t *)
                message_task_factory->instantiate(message_task_factory, NULL, NULL, NULL, 0, NULL);
            derived -> type = GIVE_ME_WORK;
            derived -> from_worker_guid = workerGuid;

            // there is no work left and the message has been sent, so turn of the worker
            // baseWorker->stop(baseWorker);
            ocrWorkerFsimXE_t* derivedWorker = (ocrWorkerFsimXE_t*)baseWorker;
            pthread_mutex_lock(&derivedWorker->isRunningMutex);
            if ( baseWorker->fctPtrs->isRunning(baseWorker) ) {
                ocrGuid_t messageTaskGuid = derived->fsimBase.base.base.guid;
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
    ocrWorker_t * ceWorker = (ocrWorker_t *) arg;
    ocrScheduler_t * ceScheduler = get_worker_scheduler(ceWorker);

    /* associate current thread with the worker */
    associate_comp_platform_and_worker(ceWorker);

    ocrGuid_t ceWorkerGuid = get_worker_guid(ceWorker);
    log_worker(INFO, "Starting scheduler routine of worker %d\n", get_worker_id(ceWorker));
    while(ceWorker->fctPtrs->isRunning(ceWorker)) {

        // pop a 'message task' to handle messaging
        // the 'state' of the scheduler should point to the message workpile
        ocrGuid_t messageTaskGuid = ceScheduler->take(ceScheduler, ceWorkerGuid);

        // TODO sagnak, oh the assumptions, clean below
        if ( NULL_GUID != messageTaskGuid ) {

            // check if the popped task is of 'message task' type
            ocrTaskFsimBase_t* currTask = NULL;
            deguidify(getCurrentPD(), messageTaskGuid, (u64*)&(currTask), NULL);

            fsim_message_interface_t* taskAsMessage = &(currTask->message_interface);
            assert(taskAsMessage->is_message(taskAsMessage) && "hoping to pop a message-task; popped non-message task");

            // TODO sagnak, we seem to 'know' the underlying task type, not sure if this is a good idea
            ocrTaskFsimMessage_t* currMessageTask = (ocrTaskFsimMessage_t*) currTask;

            // TODO sagnak, we seem to 'know' the underlying scheduler type, not sure if this is a good idea
            ocrSchedulerFsimCE_t * derivedCeScheduler = (ocrSchedulerFsimCE_t*) ceScheduler;

            // find the worker, scheduler the message originated from
            ocrGuid_t targetWorkerGuid = currMessageTask->from_worker_guid;
            ocrWorker_t* targetWorker= NULL;
            deguidify(getCurrentPD(), targetWorkerGuid, (u64*)&(targetWorker), NULL);
            ocrScheduler_t* targetScheduler = get_worker_scheduler(targetWorker);

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
                    ocrWorkerFsimXE_t* derivedWorker = (ocrWorkerFsimXE_t*)targetWorker;
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
