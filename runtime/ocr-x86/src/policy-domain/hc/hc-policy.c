
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_HC

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "policy-domain/hc/hc-policy.h"

#define DEBUG_TYPE POLICY

void hcPolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 maxCount = 0;

    // TODO: Add GUIDIFY and put release GUID in stop

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.start(policy->guidProviders[i], policy);
    }
    
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.start(policy->allocators[i], policy);
    }
    
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.start(policy->schedulers[i], policy);
    }

        
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // It is assumed that the current worker (executing this code) is
    // worker 0 and it will be started last.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 1; i < maxCount; i++) {
        policy->workers[i]->fcts.start(policy->workers[i], policy);
    }

    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    // Handle target '0'
    policy->workers[0]->fcts.start(policy->workers[0], policy);
}


void hcPolicyDomainFinish(ocrPolicyDomain_t * policy) {
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
    policy->sysProvider->fctPtrs->finish(policy->sysProvider);
}

void hcPolicyDomainStop(ocrPolicyDomain_t * policy) {

    // Finish everything in reverse order
    // In HC, we MUST call stop on the master worker first.
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
    
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.stop(policy->schedulers[i]);
    }
    
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.stop(policy->allocators[i]);
    }

    maxCount = policy->guidProviderCount;
    for(i = 0; i < maxCount; ++i) {
        policy->guidProviders[i]->fcts.stop(policy->guidProviders[i]);
    }
    policy->sysProvider->fctPtrs->stop(policy->sysProvider);
}

void hcPolicyDomainDestruct(ocrPolicyDomain_t * policy) {
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
    
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fcts.destruct(policy->schedulers[i]);
    }
    
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fcts.destruct(policy->allocators[i]);
    }

    
    // Simple hc policies don't have neighbors
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
    policy->sysProvider->fctPtrs->destruct(policy->sysProvider);

    // Destroy self
    runtimeChunkFree((u64)policy, NULL);
}

static u8 hcAllocateDb(ocrPolicyDomain_t *self, ocrFatGuid_t *guid, void** ptr, u64 size,
                       u32 properties, ocrFatGuid_t affinity, ocrInDbAllocator_t allocator) {

    // Currently a very simple model of just going through all allocators
    u64 i;
    void* result;
    for(i=0; i < self->allocatorCount; ++i) {
        result = self->allocators[i]->fcts.allocate(self->allocators[i],
                                                        size);
        if(result) break;
    }
    // TODO: return error code. Requires our own errno to be clean
    if(i < self->allocatorCount) {
        ocrDataBlock_t *block = self->dbFactories[0]->instantiate(
            self->dbFactories[0], self->allocators[i]->fguid, self->fguid,
            size, result, properties, NULL);
        *ptr = result;
        (*guid).guid = block->guid;
        (*guid).metaDataPtr = block;
        return 0;
    } 
    return 1; // TODO: Return ENOMEM
}

static u8 hcCreateEdt(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                      ocrTaskTemplate_t * edtTemplate, u32 paramc, u64* paramv,
                      u32 depc, u16 properties, ocrFatGuid_t affinity,
                      ocrFatGuid_t * outputEvent) {

    ocrTask_t * base = self->taskFactories[0]->instantiate(
        self->taskFactories[0], edtTemplate, paramc,paramv,
        depc, properties, affinity, outputEvent, NULL);
    // Check if the edt is ready to be scheduled
    if (base->depc == 0) {
        // FIXME
        self->taskFactory->fctPtrs.schedule(base);
    }
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 hcCreateEdtTemplate(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                              ocrEdt_t func, u32 paramc, u32 depc, const char* funcName) {


    ocrTaskTemplate_t *base = self->taskTemplateFactories[0]->instantiate(
        self->taskTemplateFactories[0], func, paramc, depc, funcName, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 hcCreateEvent(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                        ocrEventTypes_t type, bool takesArg) {


    ocrEvent_t *base = self->eventFactories[0]->instantiate(
        self->eventFactories[0], type, takesArg, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}


#ifdef OCR_ENABLE_STATISTICS
static ocrStats_t* hcGetStats(ocrPolicyDomain_t *self) {
    return self->statsObject;
}
#endif


static u8 hcGetGuid(ocrPolicyDomain_t *self, ocrGuid_t *guid, u64 val, ocrGuidKind type,
                    ocrPolicyCtx_t *ctx) {
    self->guidProvider->fctPtrs->getGuid(self->guidProvider, guid, val, type);
    return 0;
}

static u8 hcGetInfoForGuid(ocrPolicyDomain_t *self, ocrGuid_t guid, u64* val,
                           ocrGuidKind* type, ocrPolicyCtx_t *ctx) {
    self->guidProvider->fctPtrs->getVal(self->guidProvider, guid, val, type);
    return 0;
}

static u8 hcTakeEdt(ocrPolicyDomain_t *self, ocrCost_t *cost, u32 *count,
                ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    self->schedulers[0]->fctPtrs->takeEdt(self->schedulers[0], cost, count, edts, context);
    // When takeEdt is successful, it means there either was
    // work in the current worker's workpile, or that the scheduler
    // did work-stealing across workpiles.
    return 0;
}

static u8 hcGiveEdt(ocrPolicyDomain_t *self, u32 count, ocrGuid_t *edts, ocrPolicyCtx_t *context) {
    self->schedulers[0]->fctPtrs->giveEdt(self->schedulers[0], count, edts, context);
    return 0;
}

u8 hcPolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    u8 returnCode = 0;
    ASSERT((msg->type & PD_MSG_REQUEST) && !(msg->type & PD_MSG_RESPONSE))
    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_MEM_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_CREATE
        ASSERT(PD_MSG_FIELD(dbType) == USER_DBTYPE);
        returnCode = hcAllocateDb(self, &(PD_MSG_FIELD(guid)),
                                  &(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(size),
                                  PD_MSG_FIELD(properties),
                                  PD_MSG_FIELD(affinity),
                                  PD_MSG_FIELD(allocator));
        if(returnCode == 0) {
            ocrDataBlock_t *db= PD_MSG_FIELD(guid.metaDataPtr);
            ASSERT(db);
            // TODO: Check if properties want DB acquired
            ASSERT(db->funcId == self->dbFactories[0]->factoryId);
            PD_MSG_FIELD(ptr) = self->dbFactories[0]->fcts.acquire(
                db, PD_MSG_FIELD(edt), false);
        } else {
            // Cannot acquire
            PD_MSG_FIELD(ptr) = NULL;
        }
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_MEM_DESTROY:
    {
        // Here, we just need to free the data-block. Note that GUID
        // destruction is a separate message so we don't worry about that
        // For now, we make sure that we own the allocator and what not
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_DESTROY
        ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->guid);
        ASSERT(PD_MSG_FIELD(allocator.guid) == self->allocators[0]->fguid.guid);
        DPRINTF(DEBUG_LVL_VERB, "Freeing DB @ 0x%lx (GUID: 0x%lx)\n",
                (u64)(PD_MSG_FIELD(ptr)), PD_MSG_FIELD(guid.guid));
        self->allocators[0]->fctPtrs->free(self->allocators[0], PD_MSG_FIELD(ptr));
#undef PD_MSG
#undef PD_TYPE
        break;
    }

    case PD_MSG_MEM_ACQUIRE:
    {
        // Call the appropriate acquire function
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_ACQUIRE
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        if(db == NULL) {
            deguidify(self, PD_MSG_FIELD(guid.guid), (u64*)&db, NULL);
        }
        ASSERT(db->funcId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        self->dbFactories[0]->fcts.acquire(db, PD_MSG_FIELD(edt), false);
        
#undef PD_MSG
#undef PD_TYPE
    }
    case PD_MSG_WORK_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
        if(PD_MSG_FIELD(templateGuid.metaDataPtr) == NULL) {
            deguidify(self, PD_MSG_FIELD(templateGuid.guid),
                      (u64*)&(PD_MSG_FIELD(templateGuid.metaDataPtr)), NULL);
        }
        ASSERT(PD_MSG_FIELD(workType) == EDT_WORKTYPE);
        returnCode = hcCreateEdt(self, &(PD_MSG_FIELD(guid)),
                                 (ocrTaskTemplate_t*)PD_MSG_FIELD(templateGuid.metaDataPtr),
                                 PD_MSG_FIELD(paramc), PD_MSG_FIELD(paramv), PD_MSG_FIELD(depc),
                                 PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity),
                                 &(PD_MSG_FIELD(outputEvent)));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_WORK_DESTROY:
        ASSERT(0); // Should not be called for now
        break;
        
    case PD_MSG_EDTTEMP_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
        
        returnCode = hcCreateEdtTemplate(self, &(PD_MSG_FIELD(guid)),
                                         PD_MSG_FIELD(funcPtr), PD_MSG_FIELD(paramc),
                                         PD_MSG_FIELD(depc), PD_MSG_FIELD(funcName));
                                 
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_EDTTEMP_DESTROY:
        ASSERT(0); // Should not be called for now
        break;
        
    case PD_MSG_EVT_CREATE:
        // Event create
        break;
        
    case PD_MSG_EVT_DESTROY:
        ASSERT(0);
        break;
        
    case PD_MSG_EVT_SATISFY:
        // TODO
        break;

    case PD_MSG_GUID_CREATE:
        // TODO
        break;

    case PD_MSG_GUID_INFO:
        // TODO
        break;

    case PD_MSG_GUID_DESTROY:
        // TODO
        break;
        
    case PD_MSG_COMM_TAKE:
        ASSERT(0);
        break;
        
    case PD_MSG_COMM_GIVE:
        ASSERT(0);
        break;
        
    case PD_MSG_SYS_PRINT:
#define PD_MSG msg
#define PD_TYPE PD_MSG_SYS_PRINT
        
        self->sysProvider->print(self->sysProvider,
                                 PD_MSG_FIELD(buffer), PD_MSG_FIELD(length));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
#undef PD_MSG
#undef PD_TYPE
        
    case PD_MSG_SYS_READ:
#define PD_MSG msg
#define PD_TYPE PD_MSG_SYS_READ
        
        self->sysProvider->read(self->sysProvider, PD_MSG_FIELD(buffer),
                                PD_MSG_FIELD(length),
                                PD_MSG_FIELD(inputId));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
#undef PD_MSG
#undef PD_TYPE
        
    case PD_MSG_SYS_WRITE:
#define PD_MSG msg
#define PD_TYPE PD_MSG_SYS_WRITE
        
        self->sysProvider->write(self->sysProvider, PD_MSG_FIELD(buffer),
                                 PD_MSG_FIELD(length), PD_MSG_FIELD(outputId));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
#undef PD_MSG
#undef PD_TYPE
        
    case PD_MSG_SYS_SHUTDOWN:
        self->stop(self);
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
        
    case PD_MSG_SYS_FINISH:
        self->finish(self);
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    default:
        // Not handled
        ASSERT(0);
    }

    // This code is not needed but just shows how things would be handled (probably
    // done by sub-functions)
    if(isBlocking && (msg->type & PD_MSG_REQ_RESPONSE)) {
        ASSERT(msg->type & PD_MSG_RESPONSE); // If we were blocking and needed a response
                                             // we need to make sure there is one
    }

    return returnCode;
}

ocrPolicyDomain_t * newPolicyDomainHc(ocrPolicyDomainFactory_t * policy,
                                      ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory,
                                      ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
                                      ocrGuidProvider_t *guidProvider,
                                      ocrSys_t *sysProvider,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainHc_t * derived = (ocrPolicyDomainHc_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainHc_t), NULL);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ASSERT(base);

    base->schedulerCount = 0;
    base->allocatorCount = 0;
    base->workerCount = 0;

    base->taskFactoryCount = 0;
    base->taskTemplateFactoryCount = 0;
    base->eventFactoryCount = 0;
    base->guidProviderCount = 0;
    
    base->taskFactories = NULL;
    base->taskTemplateFactories = NULL;
    base->dbFactories = NULL;
    base->eventFactories = NULL;
    base->guidProviders = NULL;
    
    base->sysProvider = sysProvider;
#ifdef OCR_ENABLE_STATISTICS
    base->statsObject = statsObject;
#endif
    base->costFunction = costFunction;

    base->destruct = hcPolicyDomainDestruct;
    base->start = hcPolicyDomainStart;
    base->stop = hcPolicyDomainStop;
    base->finish = hcPolicyDomainFinish;
    base->processMessage = hcPolicyDomainProcessMessage;
    base->sendMessage = hcPolicyDomainSendMessage;
    base->receiveMessage = hcPolicyDomainReceiveMessage;
    base->getSys = hcPolicyDomainGetSys;
#ifdef OCR_ENABLE_STATISTICS
    base->getStats = hcGetStats;
#endif

    // no inter-policy domain for simple HC
    base->neighbors = NULL;
    base->neighborCount = 0;

    //TODO populated by ini file factories. Need setters or something ?
    base->schedulers = NULL;
    base->allocators = NULL;
    
    base->guid = UNINITIALIZED_GUID;
    return base;
}

static void destructPolicyDomainFactoryHc(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryHc(ocrParamList_t *perType) {
    ocrPolicyDomainFactoryHc_t* derived = (ocrPolicyDomainFactoryHc_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryHc_t), NULL);
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    
    ASSERT(base); // Check allocation
    
    base->instantiate = newPolicyDomainHc;
    base->destruct =  destructPolicyDomainFactoryHc;
    return base;
}

#endif /* ENABLE_POLICY_DOMAIN_HC */
