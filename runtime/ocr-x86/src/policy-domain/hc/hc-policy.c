/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include <string.h>

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "policy-domain/hc/hc-policy.h"

void hcPolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 maxCount = 0;
    
    ASSERT(policy->workerCount == policy->computeCount);

    // Start things up. We go bottom-up starting the targets (which will
    // start the platforms) and moving up from there.
    maxCount = policy->memoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->memories[i]->fctPtrs->start(policy->memories[i], policy);
    }
    
    maxCount = policy->computeCount;
    for(i = 0; i < maxCount; ++i) {
        policy->computes[i]->fctPtrs->start(policy->computes[i], policy);
    }

    maxCount = policy->workpileCount;
    for(i = 0; i < maxCount; ++i) {
        policy->workpiles[i]->fctPtrs->start(policy->workpiles[i], policy);
    }
    
    // Depends on memories
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fctPtrs->start(policy->allocators[i], policy);
    }
    
    // Depends on workpiles
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fctPtrs->start(policy->schedulers[i], policy);
    }

        
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // It is assumed that the current worker (executing this code) is
    // worker 0 and it will be started last.
    // Workers should start the underlying target and platforms
    maxCount = policy->workerCount;
    for(i = 1; i < maxCount; i++) {
        policy->workers[i]->fctPtrs->start(policy->workers[i], policy);
    }

    // Need to associate thread and worker here, as current thread fall-through
    // in user code and may need to know which Worker it is associated to.
    // Handle target '0'
    policy->workers[0]->fctPtrs->start(policy->workers[0], policy);
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
        policy->workers[i]->fctPtrs->finish(policy->workers[i]);
    }
    
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fctPtrs->finish(policy->schedulers[i]);
    }
    
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fctPtrs->finish(policy->allocators[i]);
    }

    maxCount = policy->workpileCount;
    for(i = 0; i < maxCount; ++i) {
        policy->workpiles[i]->fctPtrs->finish(policy->workpiles[i]);
    }
    
    maxCount = policy->computeCount;
    for(i = 0; i < maxCount; ++i) {
        policy->computes[i]->fctPtrs->finish(policy->computes[i]);
    }
    
    maxCount = policy->memoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->memories[i]->fctPtrs->finish(policy->memories[i]);
    }
    
    policy->guidProvider->fctPtrs->finish(policy->guidProvider);
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
        policy->workers[i]->fctPtrs->stop(policy->workers[i]);
    }
    // WARNING: Do not add code here unless you know what you're doing !!
    // If we are here, it means an EDT called ocrShutdown which
    // logically finished workers and can make thread '0' executes this
    // code before joining the other threads.

    // Thread '0' joins the other (N-1) threads.
    
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fctPtrs->stop(policy->schedulers[i]);
    }
    
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fctPtrs->stop(policy->allocators[i]);
    }

    maxCount = policy->workpileCount;
    for(i = 0; i < maxCount; ++i) {
        policy->workpiles[i]->fctPtrs->stop(policy->workpiles[i]);
    }
    
    maxCount = policy->computeCount;
    for(i = 0; i < maxCount; ++i) {
        policy->computes[i]->fctPtrs->stop(policy->computes[i]);
    }
    
    maxCount = policy->memoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->memories[i]->fctPtrs->stop(policy->memories[i]);
    }

    policy->guidProvider->fctPtrs->stop(policy->guidProvider);
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
        policy->workers[i]->fctPtrs->destruct(policy->workers[i]);
    }
    
    maxCount = policy->schedulerCount;
    for(i = 0; i < maxCount; ++i) {
        policy->schedulers[i]->fctPtrs->destruct(policy->schedulers[i]);
    }
    
    maxCount = policy->allocatorCount;
    for(i = 0; i < maxCount; ++i) {
        policy->allocators[i]->fctPtrs->destruct(policy->allocators[i]);
    }

    maxCount = policy->workpileCount;
    for(i = 0; i < maxCount; ++i) {
        policy->workpiles[i]->fctPtrs->destruct(policy->workpiles[i]);
    }
    
    maxCount = policy->computeCount;
    for(i = 0; i < maxCount; ++i) {
        policy->computes[i]->fctPtrs->destruct(policy->computes[i]);
    }
    
    maxCount = policy->memoryCount;
    for(i = 0; i < maxCount; ++i) {
        policy->memories[i]->fctPtrs->destruct(policy->memories[i]);
    }
    
    // Simple hc policies don't have neighbors
    ASSERT(policy->neighbors == NULL);

    // Destruct factories
    policy->taskFactory->destruct(policy->taskFactory);
    policy->taskTemplateFactory->destruct(policy->taskTemplateFactory);
    policy->dbFactory->destruct(policy->dbFactory);
    policy->eventFactory->destruct(policy->eventFactory);

    //Anticipate those to be null-impl for some time
    ASSERT(policy->costFunction == NULL);

    // Destroy these last in case some of the other destructs make use of them
    policy->guidProvider->fctPtrs->destruct(policy->guidProvider);
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
        result = self->allocators[i]->fctPtrs->allocate(self->allocators[i],
                                                        size);
        if(result) break;
    }
    // TODO: return error code. Requires our own errno to be clean
    if(i < self->allocatorCount) {
        ocrDataBlock_t *block = self->dbFactory->instantiate(self->dbFactory,
                                                             self->allocators[i]->guid, self->guid,
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

    ocrTask_t * base = self->taskFactory->instantiate(self->taskFactory, edtTemplate, paramc,
                                                      paramv, depc, properties, affinity,
                                                      outputEvent, NULL);
    // Check if the edt is ready to be scheduled
    if (base->depc == 0) {
        base->fctPtrs->schedule(base);
    }
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 hcCreateEdtTemplate(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                              ocrEdt_t func, u32 paramc, u32 depc, const char* funcName) {


    ocrTaskTemplate_t *base = self->taskTemplateFactory->instantiate(self->taskTemplateFactory,
                                                                     func, paramc, depc, funcName, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 hcCreateEvent(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                        ocrEventTypes_t type, bool takesArg) {


    ocrEvent_t *base = self->eventFactory->instantiate(self->eventFactory,
                                                              type, takesArg, NULL);
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
                                  PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity),
                                  PD_MSG_TYPE(allocator));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_MEM_DESTROY:
        ASSERT(0); // Should not be called for now
        break;
        
    case PD_MSG_WORK_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
        if(PD_MSG_FIELD(templateGuid.metaDataPtr) == NULL) {
            deguidify(self, PD_MSG_FIELD(templateGuid.guid),
                      &(PD_MSG_FIELD(templateGuid.metaDataPtr)), NULL);
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
        self>stop(self);
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
                                      u64 schedulerCount, u64 workerCount, u64 computeCount,
                                      u64 workpileCount, u64 allocatorCount, u64 memoryCount,
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

    base->schedulerCount = schedulerCount;
    ASSERT(schedulerCount == 1); // Simplest HC PD implementation
    base->workerCount = workerCount;
    base->computeCount = computeCount;
    base->workpileCount = workpileCount;
    base->allocatorCount = allocatorCount;
    base->memoryCount = memoryCount;

    base->taskFactory = taskFactory;
    base->taskTemplateFactory = taskTemplateFactory;
    base->dbFactory = dbFactory;
    base->eventFactory = eventFactory;
    base->guidProvider = guidProvider;
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
    base->workers = NULL;
    base->computes = NULL;
    base->workpiles = NULL;
    base->allocators = NULL;
    base->memories = NULL;

    base->guid = UNINITIALIZED_GUID;
    // TODO: Re-put the GUIDIFY thing
//    guidify(base, (u64)base, &(base->guid), OCR_GUID_POLICY);
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
