/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_XE

#include "debug.h"
#include "ocr-errors.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-runtime-types.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "policy-domain/xe/xe-policy.h"

#include "utils/profiler/profiler.h"

#define DEBUG_TYPE POLICY

#ifdef TOOL_CHAIN_XE
void xePolicyDomainStart(ocrPolicyDomain_t * policy);
#endif

void xePolicyDomainBegin(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated

    u64 i = 0;
    u64 maxCount = 0;

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.begin(policy->guidProviders[i], policy);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.begin(policy->allocators[i], policy);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.begin(policy->schedulers[i], policy);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.begin(policy->commApis[i], policy);
    }

    // REC: Moved all workers to start here.
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.begin(policy->workers[i], policy);
    }

#ifdef TOOL_CHAIN_XE
    xePolicyDomainStart(policy);
#endif
}

void xePolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // This is a bit ugly but I can't find a cleaner solution:
    //   - we need to associate the environment with the
    //   currently running worker/PD so that we can use getCurrentEnv

    u64 i = 0;
    u64 maxCount = 0;

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.start(policy->guidProviders[i], policy);
    }

    guidify(policy, (u64)policy, &(policy->fguid), OCR_GUID_POLICY);

    /*maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.start(policy->allocators[i], policy);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.start(policy->schedulers[i], policy);
    }*/

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.start(policy->commApis[i], policy);
    }

    // REC: Moved all workers to start here.
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.start(policy->workers[i], policy);
    }
}

void xePolicyDomainFinish(ocrPolicyDomain_t * policy) {
    // Finish everything in reverse order
    u64 i = 0;
    u64 maxCount = 0;

    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through and continue shutting down the
    // policy domain
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.finish(policy->workers[i]);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.finish(policy->commApis[i]);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.finish(policy->schedulers[i]);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.finish(policy->allocators[i]);
    }

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.finish(policy->guidProviders[i]);
    }

}

void xePolicyDomainStop(ocrPolicyDomain_t * policy) {

    // Finish everything in reverse order
    // In XE, we MUST call stop on the master worker first.
    // The master worker enters its work routine loop and will
    // be unlocked by ocrShutdown
    u64 i = 0;
    u64 maxCount = 0;

    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through and continue shutting down the
    // policy domain
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.stop(policy->workers[i]);
    }
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means an EDT called ocrShutdown which
    // logically finished workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.stop(policy->commApis[i]);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.stop(policy->schedulers[i]);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.stop(policy->allocators[i]);
    }

    // We could release our GUID here but not really required

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.stop(policy->guidProviders[i]);
    }
}

void xePolicyDomainDestruct(ocrPolicyDomain_t * policy) {
    // Destroying instances
    u64 i = 0;
    u64 maxCount = 0;

    // Note: As soon as worker '0' is stopped; its thread is
    // free to fall-through and continue shutting down the
    // policy domain
    maxCount = policy->workerCount;
    for(i = 0; i < maxCount; i++) {
        policy->workers[i]->fcts.destruct(policy->workers[i]);
    }

    maxCount = policy->commApiCount;
    for(i = 0; i < maxCount; i++) {
        policy->commApis[i]->fcts.destruct(policy->commApis[i]);
    }

    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.destruct(policy->schedulers[i]);
    }

    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.destruct(policy->allocators[i]);
    }


    // Simple xe policies don't have neighbors
    ASSERT(policy->neighbors == NULL);

    // Destruct factories
    maxCount = policy->taskFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->taskFactories[i]->destruct(policy->taskFactories[i]);
    }

    maxCount = policy->taskTemplateFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->taskTemplateFactories[i]->destruct(policy->taskTemplateFactories[i]);
    }

    maxCount = policy->dbFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->dbFactories[i]->destruct(policy->dbFactories[i]);
    }

    maxCount = policy->eventFactoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->eventFactories[i]->destruct(policy->eventFactories[i]);
    }

    //Anticipate those to be null-impl for some time
    ASSERT(policy->costFunction == NULL);

    // Destroy these last in case some of the other destructs make use of them
    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.destruct(policy->guidProviders[i]);
    }

    // Destroy self
    runtimeChunkFree((u64)policy->workers, NULL);
    runtimeChunkFree((u64)policy->schedulers, NULL);
    runtimeChunkFree((u64)policy->allocators, NULL);
    runtimeChunkFree((u64)policy->taskFactories, NULL);
    runtimeChunkFree((u64)policy->taskTemplateFactories, NULL);
    runtimeChunkFree((u64)policy->dbFactories, NULL);
    runtimeChunkFree((u64)policy->eventFactories, NULL);
    runtimeChunkFree((u64)policy->guidProviders, NULL);
    runtimeChunkFree((u64)policy, NULL);
}

static void localDeguidify(ocrPolicyDomain_t *self, ocrFatGuid_t *guid) {
    ASSERT(self->guidProviderCount == 1);
    if(guid->guid != NULL_GUID && guid->guid != UNINITIALIZED_GUID) {
        if(guid->metaDataPtr == NULL) {
            self->guidProviders[0]->fcts.getVal(self->guidProviders[0], guid->guid,
                                                (u64*)(&(guid->metaDataPtr)), NULL);
        }
    }
}

static ocrFatGuid_t * getTaskGuidBuffer(ocrPolicyDomain_t *self, u32 count) {
    ocrPolicyDomainXe_t * derived = (ocrPolicyDomainXe_t *)self;
    ASSERT(derived->taskCounter < MAX_XE_TASK);
    ocrFatGuid_t * bufPtr = &(derived->fguidPool[derived->taskCounter]);
    derived->taskCounter += count;
    return bufPtr;
}

static u8 xeProcessCeRequest(ocrPolicyDomain_t *self, ocrPolicyMsg_t **msg) {
    u8 returnCode = 0;
    u32 type = ((*msg)->type & PD_MSG_TYPE_ONLY);
    ASSERT((*msg)->type & PD_MSG_REQUEST);
    if ((*msg)->type & PD_MSG_REQ_RESPONSE) {
        ocrMsgHandle_t *handle = NULL;
        returnCode = self->fcts.sendMessage(self, self->parentLocation, (*msg),
                                            &handle, (TWOWAY_MSG_PROP | PERSIST_MSG_PROP));
        if (returnCode == 0) {
            ASSERT(handle && handle->msg);
            ASSERT(handle->msg == *msg); // This is what we passed in
            RESULT_ASSERT(self->fcts.waitMessage(self, &handle), ==, 0);
            ASSERT(handle->response);
            // Check if the message was a proper response and came from the right place
            ASSERT((handle->response->type & PD_MSG_TYPE_ONLY) == type);
            ASSERT(handle->response->srcLocation == self->parentLocation);
            ASSERT(handle->response->destLocation == self->myLocation);
            // The call provides no guarantee that the message in response
            // will use the same buffer (msg). For now, we don't handle that
            // case. If we needed to do it, we would need to copy response
            // into *msg.
            ASSERT(handle->response == *msg); // If this is not the case, we need to copy back in msg
            handle->destruct(handle);
        }
    } else {
        returnCode = self->fcts.sendMessage(self, self->parentLocation, (*msg), NULL, 0);
    }
    return returnCode;
}

static u8 xeProcessCeResponse(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg) {
    ASSERT(msg->type & PD_MSG_REQ_RESPONSE);
    msg->type &= ~PD_MSG_REQUEST;
    msg->type |=  PD_MSG_RESPONSE;
    RESULT_ASSERT(self->fcts.sendMessage(self, msg->srcLocation, msg, NULL, 0), ==, 0);
    return 0;
}

u8 xePolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    u8 returnCode = 0;
    ASSERT((msg->type & PD_MSG_REQUEST) && (!(msg->type & PD_MSG_RESPONSE)));

    switch(msg->type & PD_MSG_TYPE_ONLY) {
    // First type of messages: things that we offload completely to the CE
    case PD_MSG_DB_CREATE: case PD_MSG_DB_DESTROY:
    case PD_MSG_DB_ACQUIRE: case PD_MSG_DB_RELEASE: case PD_MSG_DB_FREE:
    case PD_MSG_MEM_ALLOC: case PD_MSG_MEM_UNALLOC:
    case PD_MSG_WORK_CREATE: case PD_MSG_WORK_DESTROY:
    case PD_MSG_EDTTEMP_CREATE: case PD_MSG_EDTTEMP_DESTROY:
    case PD_MSG_EVT_CREATE: case PD_MSG_EVT_DESTROY: case PD_MSG_EVT_GET:
    case PD_MSG_GUID_CREATE: case PD_MSG_GUID_INFO: case PD_MSG_GUID_DESTROY:
    case PD_MSG_COMM_TAKE: case PD_MSG_COMM_GIVE:
    case PD_MSG_DEP_ADD: case PD_MSG_DEP_REGSIGNALER: case PD_MSG_DEP_REGWAITER:
    case PD_MSG_DEP_SATISFY: {

        START_PROFILE(pd_xe_OffloadtoCE);
        if((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_COMM_GIVE) {
            START_PROFILE(pd_xe_Give);
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_GIVE
            ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
            ocrFatGuid_t * guidBuf = getTaskGuidBuffer(self, PD_MSG_FIELD(guidCount));
            u32 i;
            for (i = 0; i < PD_MSG_FIELD(guidCount); i++)
                guidBuf[i] = PD_MSG_FIELD(guids)[i];
            PD_MSG_FIELD(guids) = guidBuf;
#undef PD_MSG
#undef PD_TYPE
            EXIT_PROFILE;
        }

        DPRINTF(DEBUG_LVL_VERB, "XE Policy offloading message of type 0x%x to CE\n",
                msg->type & PD_MSG_TYPE_ONLY);
        returnCode = xeProcessCeRequest(self, &msg);

        if((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_COMM_TAKE) {
            START_PROFILE(pd_xe_Take);
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_TAKE
            if (PD_MSG_FIELD(guidCount) > 0) {
                DPRINTF(DEBUG_LVL_INFO, "[XE%lu] (%lu) Received EDT guid: 0x%lx metadata: 0x%p\n",
                        (u64)self->myLocation, (u64)msg->srcLocation, (PD_MSG_FIELD(guids))->guid,
                        (PD_MSG_FIELD(guids))->metaDataPtr);
                localDeguidify(self, (PD_MSG_FIELD(guids)));
                // For now, we return the execute function for EDTs
                PD_MSG_FIELD(extra) = (u64)(self->taskFactories[0]->fcts.execute);
            }
#undef PD_MSG
#undef PD_TYPE
            EXIT_PROFILE;
        }
        EXIT_PROFILE;
        break;
    }

    // Messages are not handled at all
    case PD_MSG_WORK_EXECUTE: case PD_MSG_DEP_UNREGSIGNALER:
    case PD_MSG_DEP_UNREGWAITER: case PD_MSG_SAL_PRINT:
    case PD_MSG_SAL_READ: case PD_MSG_SAL_WRITE:
    case PD_MSG_MGT_REGISTER: case PD_MSG_MGT_UNREGISTER:
    case PD_MSG_SAL_TERMINATE:
    {
        DPRINTF(DEBUG_LVL_WARN, "XE PD does not handle call of type 0x%x\n",
                (u32)(msg->type & PD_MSG_TYPE_ONLY));
        ASSERT(0);
        returnCode = OCR_ENOTSUP;
        break;
    }

    // Messages handled locally
    case PD_MSG_DEP_DYNADD: {
        START_PROFILE(pd_xe_DepDynAdd);
        ocrTask_t *curTask = NULL;
        getCurrentEnv(NULL, NULL, &curTask, NULL);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_DYNADD
        // Check to make sure that the EDT is only doing this to
        // itself
        // Also, this should only happen when there is an actual EDT
        ASSERT(curTask &&
               curTask->guid == PD_MSG_FIELD(edt.guid));

        ASSERT(curTask->fctId == self->taskFactories[0]->factoryId);
        PD_MSG_FIELD(properties) = self->taskFactories[0]->fcts.notifyDbAcquire(curTask, PD_MSG_FIELD(db));
#undef PD_MSG
#undef PD_TYPE
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_DEP_DYNREMOVE: {
        START_PROFILE(pd_xe_DepDynRemove);
        ocrTask_t *curTask = NULL;
        getCurrentEnv(NULL, NULL, &curTask, NULL);
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_DYNREMOVE
        // Check to make sure that the EDT is only doing this to
        // itself
        // Also, this should only happen when there is an actual EDT
        ASSERT(curTask &&
            curTask->guid == PD_MSG_FIELD(edt.guid));

        ASSERT(curTask->fctId == self->taskFactories[0]->factoryId);
        PD_MSG_FIELD(properties) = self->taskFactories[0]->fcts.notifyDbRelease(curTask, PD_MSG_FIELD(db));
#undef PD_MSG
#undef PD_TYPE
        break;
    }

    case PD_MSG_MGT_SHUTDOWN: {
        START_PROFILE(pd_xe_Shutdown);
        self->fcts.stop(self);
        if(msg->srcLocation == self->myLocation) {
            returnCode = xeProcessCeRequest(self, &msg);
        } else {
            returnCode = xeProcessCeResponse(self, msg);
        }
        EXIT_PROFILE;
        break;
    }

    case PD_MSG_MGT_FINISH: {
        START_PROFILE(pd_xe_Finish);
        self->fcts.finish(self);
        EXIT_PROFILE;
        break;
    }
    default: {
        DPRINTF(DEBUG_LVL_WARN, "Unknown message type 0x%x\n", (u32)(msg->type & PD_MSG_TYPE_ONLY));
        ASSERT(0);
    }
    }; // End of giant switch

    return returnCode;
}

u8 xePdSendMessage(ocrPolicyDomain_t* self, ocrLocation_t target, ocrPolicyMsg_t *message,
                   ocrMsgHandle_t **handle, u32 properties) {
    ASSERT(target == self->parentLocation);
    // Set the header of this message
    message->srcLocation  = self->myLocation;
    message->destLocation = self->parentLocation;
    u8 returnCode = self->commApis[0]->fcts.sendMessage(self->commApis[0], target, message, handle, properties);
    if (returnCode == 0) return 0;
    switch (returnCode) {
    case OCR_EBUSY:
    case OCR_ECANCELED: {
        // Our outgoing message was cancelled and we probably have an incoming
        // reason why (shutdown most likely)
        // We destruct the handle for the first message in case it was partially used
        if(*handle)
            (*handle)->destruct(*handle);
        ocrMsgHandle_t *tempHandle = NULL;
        RESULT_ASSERT(self->fcts.waitMessage(self, &tempHandle), ==, 0);
        ASSERT(tempHandle && tempHandle->response);
        ocrPolicyMsg_t * newMsg = tempHandle->response;
        ASSERT(newMsg->srcLocation == self->parentLocation);
        ASSERT(newMsg->destLocation == self->myLocation);
        RESULT_ASSERT(self->fcts.processMessage(self, newMsg, true), ==, 0);
        tempHandle->destruct(tempHandle);
    }
        break;
    default:
        ASSERT(0);
    }
    return returnCode;
}

u8 xePdPollMessage(ocrPolicyDomain_t *self, ocrMsgHandle_t **handle) {
    return self->commApis[0]->fcts.pollMessage(self->commApis[0], handle);
}

u8 xePdWaitMessage(ocrPolicyDomain_t *self,  ocrMsgHandle_t **handle) {
    return self->commApis[0]->fcts.waitMessage(self->commApis[0], handle);
}

void* xePdMalloc(ocrPolicyDomain_t *self, u64 size) {
    START_PROFILE(pd_xe_pdMalloc);
    void *ptr;
    ocrPolicyMsg_t msg;
    ocrPolicyMsg_t* pmsg = &msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_ALLOC
    msg.type = PD_MSG_MEM_ALLOC  | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(type) = DB_MEMTYPE;
    PD_MSG_FIELD(size) = size;
    ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
    u8 msgResult = xeProcessCeRequest(self, &pmsg);
    ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
    ptr = PD_MSG_FIELD(ptr);
#undef PD_TYPE
#undef PD_MSG
    RETURN_PROFILE(ptr);
}

void xePdFree(ocrPolicyDomain_t *self, void* addr) {
    START_PROFILE(pd_xe_pdFree);
    ocrPolicyMsg_t msg;
    ocrPolicyMsg_t *pmsg = &msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_UNALLOC
    msg.type = PD_MSG_MEM_UNALLOC | PD_MSG_REQUEST;
    PD_MSG_FIELD(ptr) = addr;
    PD_MSG_FIELD(type) = DB_MEMTYPE;
    // TODO: Things are missing. Brian's new way to free things should fix this!
    PD_MSG_FIELD(properties) = 0;
    u8 msgResult = xeProcessCeRequest(self, &pmsg);
    ASSERT(msgResult == 0);
#undef PD_MSG
#undef PD_TYPE
    RETURN_PROFILE();
}

ocrPolicyDomain_t * newPolicyDomainXe(ocrPolicyDomainFactory_t * factory,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainXe_t * derived = (ocrPolicyDomainXe_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainXe_t), NULL);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ASSERT(base);
#ifdef OCR_ENABLE_STATISTICS
    factory->initialize(factory, base, statsObject, costFunction, perInstance);
#else
    factory->initialize(factory, base, costFunction, perInstance);
#endif

    return base;
}

void initializePolicyDomainXe(ocrPolicyDomainFactory_t * factory, ocrPolicyDomain_t* self,
#ifdef OCR_ENABLE_STATISTICS
                              ocrStats_t *statsObject,
#endif
                              ocrCost_t *costFunction, ocrParamList_t *perInstance) {
#ifdef OCR_ENABLE_STATISTICS
    self->statsObject = statsObject;
#endif

    initializePolicyDomainOcr(factory, self, perInstance);
}

static void destructPolicyDomainFactoryXe(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryXe(ocrParamList_t *perType) {
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryXe_t), (void *)1);

    ASSERT(base); // Check allocation

#ifdef OCR_ENABLE_STATISTICS
    base->instantiate = FUNC_ADDR(ocrPolicyDomain_t*(*)(ocrPolicyDomainFactory_t*,ocrCost_t*,
                                  ocrParamList_t*), newPolicyDomainXe);
#endif

    base->instantiate = &newPolicyDomainXe;
    base->initialize = &initializePolicyDomainXe;
    base->destruct = &destructPolicyDomainFactoryXe;

    base->policyDomainFcts.destruct = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), xePolicyDomainDestruct);
    base->policyDomainFcts.begin = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), xePolicyDomainBegin);
    base->policyDomainFcts.start = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), xePolicyDomainStart);
    base->policyDomainFcts.stop = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), xePolicyDomainStop);
    base->policyDomainFcts.finish = FUNC_ADDR(void(*)(ocrPolicyDomain_t*), xePolicyDomainFinish);
    base->policyDomainFcts.processMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*,ocrPolicyMsg_t*,u8), xePolicyDomainProcessMessage);
    base->policyDomainFcts.sendMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*, ocrLocation_t, ocrPolicyMsg_t*, ocrMsgHandle_t**, u32),
                                         xePdSendMessage);
    base->policyDomainFcts.pollMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), xePdPollMessage);
    base->policyDomainFcts.waitMessage = FUNC_ADDR(u8(*)(ocrPolicyDomain_t*, ocrMsgHandle_t**), xePdWaitMessage);

    base->policyDomainFcts.pdMalloc = FUNC_ADDR(void*(*)(ocrPolicyDomain_t*,u64), xePdMalloc);
    base->policyDomainFcts.pdFree = FUNC_ADDR(void(*)(ocrPolicyDomain_t*,void*), xePdFree);
    return base;
}

#endif /* ENABLE_POLICY_DOMAIN_XE */
