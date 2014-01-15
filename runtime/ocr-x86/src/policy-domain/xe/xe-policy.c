
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

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "policy-domain/xe/xe-policy.h"

#define DEBUG_TYPE POLICY

void xePolicyDomainStart(ocrPolicyDomain_t * policy) {
    // The PD should have been brought up by now and everything instantiated
    // WARNING: Threads start should be the last thing we do here after
    //          all data-structures have been initialized.
    u64 i = 0;
    u64 maxCount = 0;

    guidify(policy, (u64)policy, &(policy->fguid), OCR_GUID_POLICY);

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

    // REC: Moved all workers to start here. 
    // Note: it's important to first logically start all workers.
    // Once they are all up, start the runtime.
    // It is assumed that the current worker (executing this code) is
    // worker 0 and it will be started last.
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

    policy->salProvider->fcts.finish(policy->salProvider);
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
    policy->salProvider->fcts.stop(policy->salProvider);
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
    
    policy->salProvider->fcts.destruct(policy->salProvider);

    // Destroy self
    runtimeChunkFree((u64)policy, NULL);
}

static void localDeguidify(ocrPolicyDomain_t *self, ocrFatGuid_t *guid) {
    // TODO
}

// In all these functions, we consider only a single PD. In other words, in XE, we
// deal with everything locally and never send messages

static u8 xeCreateEdt(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                      ocrFatGuid_t  edtTemplate, u32 paramc, u64* paramv,
                      u32 depc, u32 properties, ocrFatGuid_t affinity,
                      ocrFatGuid_t * outputEvent) {

    
    ocrTaskTemplate_t *taskTemplate = (ocrTaskTemplate_t*)edtTemplate.metaDataPtr;
    
    ASSERT(((taskTemplate->paramc == EDT_PARAM_UNK) && paramc != EDT_PARAM_DEF) ||
           (taskTemplate->paramc != EDT_PARAM_UNK && (paramc == EDT_PARAM_DEF ||
                                                      taskTemplate->paramc == paramc)));
    ASSERT(((taskTemplate->depc == EDT_PARAM_UNK) && depc != EDT_PARAM_DEF) ||
           (taskTemplate->depc != EDT_PARAM_UNK && (depc == EDT_PARAM_DEF ||
                                                    taskTemplate->depc == depc)));

    if(paramc == EDT_PARAM_DEF) {
        paramc = taskTemplate->paramc;
    }
    if(depc == EDT_PARAM_DEF) {
        depc = taskTemplate->depc;
    }
    // If paramc are expected, double check paramv is not NULL
    if((paramc > 0) && (paramv == NULL))
        return OCR_EINVAL;

    ocrTask_t * base = self->taskFactories[0]->instantiate(
        self->taskFactories[0], edtTemplate, paramc, paramv,
        depc, properties, affinity, outputEvent, NULL);
    
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 xeCreateEdtTemplate(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                              ocrEdt_t func, u32 paramc, u32 depc, const char* funcName) {


    ocrTaskTemplate_t *base = self->taskTemplateFactories[0]->instantiate(
        self->taskTemplateFactories[0], func, paramc, depc, funcName, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

static u8 xeCreateEvent(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                        ocrEventTypes_t type, bool takesArg) {

    ocrEvent_t *base = self->eventFactories[0]->instantiate(
        self->eventFactories[0], type, takesArg, NULL);
    (*guid).guid = base->guid;
    (*guid).metaDataPtr = base;
    return 0;
}

#ifdef OCR_ENABLE_STATISTICS
static ocrStats_t* xeGetStats(ocrPolicyDomain_t *self) {
    return self->statsObject;
}
#endif

u8 xePolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    u8 returnCode = 0;
    ASSERT((msg->type & PD_MSG_REQUEST) && !(msg->type & PD_MSG_RESPONSE))
    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_DB_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_CREATE
        // TODO: Add properties whether DB needs to be acquired or not
        // This would impact where we do the PD_MSG_MEM_ALLOC for example
        ASSERT(PD_MSG_FIELD(dbType) == USER_DBTYPE); // Assert this is a user-requested data block.
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        msg->srcLocation  = myLocation;
        msg->destLocation = parentLocation;
        ocrPolicyMsg_t *msgAddressSent = msg;      // Jot down address of message, for comparison.
        u8 msgResult;
        msgResult = self->workers[0]->sendMessage( // Pass msg to CE.
            self->workers[0],      // Input: ocrWorker_t * worker; (i.e. this xe's curr worker)
            parentLocation,        // Input: ocrLocation_t target; (location of parent ce)
            &msg);                 // In/Out: ocrPolicyMsg_t **msg; (msg to process, and its response)
        ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
        msgResult = self->workers[0]->waitMessage( // Pass msg to CE.
            self->workers[0],      // Input: ocrWorker_t * worker; (i.e. this xe's curr worker)
            &msg);                 // In/Out: ocrPolicyMsg_t **msg; (msg to process, and its response)
        ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
        ASSERT (msg == msgAddressSent);  // If fails, must handle CE returning different message addr
#if 0
The following code is kicked to the curb in favor of the above code to forward the request to the CE.
        returnCode = xeAllocateDb(self, &(PD_MSG_FIELD(guid)),
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
            PD_MSG_FIELD(properties) = 0;
        } else {
            // Cannot acquire
            PD_MSG_FIELD(ptr) = NULL;
            PD_MSG_FIELD(properties) = returnCode;
        }
#endif
// TODO:  The following line probably should be deleted, as it is probably already done by the CE.
//      msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE); // Convert msg type for Request to Response.
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_DB_DESTROY:
    {
        // Should never ever be called. The user calls free and internally
        // this will call whatever it needs (most likely PD_MSG_MEM_UNALLOC)
        // This would get called when DBs move for example
        ASSERT(0);
        break;
    }

    case PD_MSG_DB_ACQUIRE:
    {
        // Call the appropriate acquire function
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_ACQUIRE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->funcId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        msg->srcLocation  = myLocation;
        msg->destLocation = parentLocation;
        ocrPolicyMsg_t *msgAddressSent = msg;      // Jot down address of message, for comparison.
        u8 msgResult;
        msgResult = self->workers[0]->sendMessage( // Pass msg to CE.
            self->workers[0],      // Input: ocrWorker_t * worker; (i.e. this xe's curr worker)
            parentLocation,        // Input: ocrLocation_t target; (location of parent ce)
            &msg);                 // In/Out: ocrPolicyMsg_t **msg; (msg to process, and its response)
        ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
        msgResult = self->workers[0]->waitMessage( // Pass msg to CE.
            self->workers[0],      // Input: ocrWorker_t * worker; (i.e. this xe's curr worker)
            &msg);                 // In/Out: ocrPolicyMsg_t **msg; (msg to process, and its response)
        ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
        ASSERT (msg == msgAddressSent);  // If fails, must handle CE returning different message addr
#if 0
The following code is kicked to the curb in favor of the above code to forward the request to the CE.
        PD_MSG_FIELD(ptr) =
            self->dbFactories[0]->fcts.acquire(db, PD_MSG_FIELD(edt), PD_MSG_FIELD(properties) & 1);
#endif
#undef PD_MSG
#undef PD_TYPE
// TODO:  The following line probably should be deleted, as it is probably already done by the CE.
//      msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE); // Convert msg type for Request to Response.
        break;
    }

    case PD_MSG_DB_RELEASE:
    {
        // Call the appropriate release function
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_RELEASE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->funcId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        msg->srcLocation  = myLocation;
        msg->destLocation = parentLocation;
        ocrPolicyMsg_t *msgAddressSent = msg;      // Jot down address of message, for comparison.
        u8 msgResult;
        msgResult = self->workers[0]->sendMessage( // Pass msg to CE.
            self->workers[0],      // Input: ocrWorker_t * worker; (i.e. this xe's curr worker)
            parentLocation,        // Input: ocrLocation_t target; (location of parent ce)
            &msg);                 // In/Out: ocrPolicyMsg_t **msg; (msg to process, and its response)
        ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
        msgResult = self->workers[0]->waitMessage( // Pass msg to CE.
            self->workers[0],      // Input: ocrWorker_t * worker; (i.e. this xe's curr worker)
            &msg);                 // In/Out: ocrPolicyMsg_t **msg; (msg to process, and its response)
        ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
        ASSERT (msg == msgAddressSent);  // If fails, must handle CE returning different message addr
#if 0
The following code is kicked to the curb in favor of the above code to forward the request to the CE.
        PD_MSG_FIELD(properties) = 
            self->dbFactories[0]->fcts.release(db, PD_MSG_FIELD(edt), PD_MSG_FIELD(properties) & 1);
#endif
#undef PD_MSG
#undef PD_TYPE
// TODO:  The following line probably should be deleted, as it is probably already done by the CE.
//      msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE); // Convert msg type for Request to Response.
        break;
    }

    case PD_MSG_DB_FREE:
    {
        // Call the appropriate free function
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_FREE
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        localDeguidify(self, &(PD_MSG_FIELD(edt)));
        ocrDataBlock_t *db = (ocrDataBlock_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(db->funcId == self->dbFactories[0]->factoryId);
        ASSERT(!(msg->type & PD_MSG_REQ_RESPONSE));
        PD_MSG_FIELD(properties) =
            self->dbFactories[0]->fcts.free(db, PD_MSG_FIELD(edt));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }

    case PD_MSG_MEM_ALLOC:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_ALLOC
        ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);
        PD_MSG_FIELD(allocatingPD.metaDataPtr) = self;
        PD_MSG_FIELD(properties) =
            xeMemAlloc(self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(size),
                       &(PD_MSG_FIELD(ptr)));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }

    case PD_MSG_MEM_UNALLOC:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_UNALLOC
        ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);
        PD_MSG_FIELD(allocatingPD.metaDataPtr) = self;
        PD_MSG_FIELD(properties) =
            xeMemUnAlloc(self, &(PD_MSG_FIELD(allocator)), PD_MSG_FIELD(ptr));
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_WORK_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_WORK_CREATE
        localDeguidify(self, &(PD_MSG_FIELD(templateGuid)));
        localDeguidify(self, &(PD_MSG_FIELD(affinity)));
        ocrFatGuid_t *outputEvent = NULL;
        if(PD_MSG_FIELD(outputEvent.guid) == UNINITIALIZED_GUID) {
            outputEvent = &(PD_MSG_FIELD(outputEvent));
        }
        ASSERT(PD_MSG_FIELD(workType) == EDT_WORKTYPE);
        PD_MSG_FIELD(properties) = xeCreateEdt(
            self, &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(templateGuid),
            PD_MSG_FIELD(paramc), PD_MSG_FIELD(paramv), PD_MSG_FIELD(depc),
            PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity), outputEvent);
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_WORK_EXECUTE:
    {
        ASSERT(0); // Not used for this PD
        break;
    }
    
    case PD_MSG_WORK_DESTROY:
    {
        // TODO: FIXME: Could be called directly by user but
        // we do not implement it just yet
        ASSERT(0);
        break;
    }
    
    case PD_MSG_EDTTEMP_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
        
        returnCode = xeCreateEdtTemplate(self, &(PD_MSG_FIELD(guid)),
                                         PD_MSG_FIELD(funcPtr), PD_MSG_FIELD(paramc),
                                         PD_MSG_FIELD(depc), PD_MSG_FIELD(funcName));
                                 
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
#undef PD_MSG
#undef PD_TYPE
        break;
    }
    
    case PD_MSG_EDTTEMP_DESTROY:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_EDTTEMP_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrTaskTemplate_t *tTemplate = (ocrTaskTemplate_t*)(PD_MSG_FIELD(guid.metaDataPtr));
        ASSERT(tTemplate->fctId == self->taskTemplateFactories[0]->factoryId);
        self->taskTemplateFactories[0]->fcts.destruct(tTemplate);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST);
        break;
    }
    
    case PD_MSG_EVT_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_CREATE
        PD_MSG_FIELD(properties) = xeCreateEvent(self, &(PD_MSG_FIELD(guid)),
                                                 PD_MSG_FIELD(type), PD_MSG_FIELD(properties) & 1);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
        
    case PD_MSG_EVT_DESTROY:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrEvent_t *evt = (ocrEvent_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        self->eventFactories[0]->fcts[evt->kind].destruct(evt);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST);
        break;            
    }

    case PD_MSG_EVT_GET:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_EVT_GET
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        ocrEvent_t *evt = (ocrEvent_t*)PD_MSG_FIELD(guid.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        PD_MSG_FIELD(data) = self->eventFactories[0]->fcts[evt->kind].get(evt);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
    
    case PD_MSG_GUID_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_CREATE
        if(PD_MSG_FIELD(size) != 0) {
            // Here we need to create a metadata area as well
            PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.createGuid(
                self->guidProviders[0], &(PD_MSG_FIELD(guid)), PD_MSG_FIELD(size),
                PD_MSG_FIELD(kind));
        } else {
            // Here we just need to associate a GUID
            ocrGuid_t temp;
            PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.getGuid(
                self->guidProviders[0], &temp, (u64)PD_MSG_FIELD(guid.metaDataPtr),
                PD_MSG_FIELD(kind));
            PD_MSG_FIELD(guid.guid) = temp;
        }
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }

    case PD_MSG_GUID_INFO:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_INFO
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        if(PD_MSG_FIELD(properties) & KIND_GUIDPROP) {
            self->guidProviders[0]->fcts.getKind(self->guidProviders[0],
                PD_MSG_FIELD(guid.guid), &(PD_MSG_FIELD(kind)));
            PD_MSG_FIELD(properties) = KIND_GUIDPROP | WMETA_GUIDPROP | RMETA_GUIDPROP;
        } else {
            PD_MSG_FIELD(properties) = WMETA_GUIDPROP | RMETA_GUIDPROP;
        }
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
    
    case PD_MSG_GUID_DESTROY:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_GUID_DESTROY
        localDeguidify(self, &(PD_MSG_FIELD(guid)));
        PD_MSG_FIELD(properties) = self->guidProviders[0]->fcts.releaseGuid(
            self->guidProviders[0], PD_MSG_FIELD(guid), PD_MSG_FIELD(properties) & 1);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
        
    case PD_MSG_COMM_TAKE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_TAKE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        // XE currently has no scheduler. So, it sends a message to CE asking for work.
        PD_MSG_FIELD(properties) = self->workers[0]->fcts.sendMessage(
            self->workers[0], self->parentLocation, &msg); 
        // For now, we return the execute function for EDTs
        PD_MSG_FIELD(extra) = (u64)(self->taskFactories[0]->fcts.execute);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
        
    case PD_MSG_COMM_GIVE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_GIVE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        PD_MSG_FIELD(properties) = self->schedulers[0]->fcts.giveEdt(
            self->schedulers[0], &(PD_MSG_FIELD(guidCount)),
            PD_MSG_FIELD(guids));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }

    case PD_MSG_DEP_ADD:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_ADD
        // We first get information about the source and destination
        ocrGuidKind srcKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(source.guid),
            (u64*)(&(PD_MSG_FIELD(source.metaDataPtr))), &srcKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);

        ocrFatGuid_t src = PD_MSG_FIELD(source);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);
        if(srcKind == OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(src.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            self->eventFactories[0]->fcts[evt->kind].registerWaiter(
                evt, dest, PD_MSG_FIELD(slot));
        } else {
            // Some sanity check
            ASSERT(srcKind == OCR_GUID_DB || srcKind == OCR_GUID_EDT);
        }
        if(dstKind == OCR_GUID_EDT) {
            ocrTask_t *task = (ocrTask_t*)(dest.metaDataPtr);
            ASSERT(task->fctId == self->taskFactories[0]->factoryId);
            self->taskFactories[0]->fcts.registerSignaler(task, src, PD_MSG_FIELD(slot));
        } else if(dstKind == OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                evt, src, PD_MSG_FIELD(slot));
        } else {
            ASSERT(0); // Cannot have other types of destinations
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
    statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }

    case PD_MSG_DEP_REGSIGNALER:
    {
        // Never used for now
        ASSERT(0);
        break;
    }

    case PD_MSG_DEP_REGWAITER:
    {
        // Never used for now
        ASSERT(0);
        break;
    }

    case PD_MSG_DEP_SATISFY:
    {
        // TODO
    }

    case PD_MSG_DEP_UNREGSIGNALER:
    {
        // Never used for now
        ASSERT(0);
        break;
    }

    case PD_MSG_DEP_UNREGWAITER:
    {
        // Never used for now
        ASSERT(0);
        break;
    }
        
    case PD_MSG_SAL_PRINT:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_SAL_PRINT
        self->salProvider->fcts.print(self->salProvider, PD_MSG_FIELD(buffer),
                                 PD_MSG_FIELD(length));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
        
    case PD_MSG_SAL_READ:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_SAL_READ
        self->salProvider->fcts.read(self->salProvider, PD_MSG_FIELD(buffer), PD_MSG_FIELD(length),
                                PD_MSG_FIELD(inputId));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
    
    case PD_MSG_SAL_WRITE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_SAL_WRITE
        self->salProvider->fcts.write(self->salProvider, PD_MSG_FIELD(buffer),
                                 PD_MSG_FIELD(length), PD_MSG_FIELD(outputId));
#undef PD_MSG
#undef PD_TYPE
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);            
        break;
    }
        
    case PD_MSG_MGT_SHUTDOWN:
    {
        self->stop(self);
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }
        
    case PD_MSG_MGT_FINISH:
    {
        self->finish(self);
        msg->type &= (~PD_MSG_REQUEST | PD_MSG_RESPONSE);
        break;
    }

    case PD_MSG_MGT_REGISTER:
    {
        // Only one PD at this time
        ASSERT(0);
        break;
    }

    case PD_MSG_MGT_UNREGISTER:
    {
        // Only one PD at this time
        ASSERT(0);
        break;
    }
    
    default:
        // Not handled
        ASSERT(0);
    }

    // If we were blocking and needed a response we need to make sure there is one
    if(isBlocking && (msg->type & PD_MSG_REQ_RESPONSE)) {
        ASSERT(msg->type & PD_MSG_RESPONSE); 
        returnCode = self->workers[0]->fcts.waitMessage(
            self->workers[0], self->parentLocation, &msg); 
        PD_MSG_FIELD(properties) = returnCode;
    }
    return returnCode;
}

void* xePdMalloc(ocrPolicyDomain_t *self, u64 size) {
    // TODO
    return NULL;
}

void xePdFree(ocrPolicyDomain_t *self, void* addr) {
    // TODO
}

ocrPolicyDomain_t * newPolicyDomainXe(ocrPolicyDomainFactory_t * policy,
                                      u64 schedulerCount, u64 allocatorCount, u64 workerCount,
                                      ocrTaskFactory_t *taskFactory, ocrTaskTemplateFactory_t *taskTemplateFactory,
                                      ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
                                      ocrGuidProvider_t *guidProvider,
                                      ocrSal_t *salProvider,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainXe_t * derived = (ocrPolicyDomainXe_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainXe_t), NULL);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ASSERT(base);
	ASSERT(schedulerCount == 0);
	ASSERT(workerCount == 1);

    base->schedulerCount = schedulerCount;
    base->allocatorCount = allocatorCount;
    base->workerCount = workerCount;

    base->taskFactoryCount = 0;
    base->taskTemplateFactoryCount = 0;
    base->eventFactoryCount = 0;
    base->guidProviderCount = 0;
    
    base->taskFactories = NULL;
    base->taskTemplateFactories = NULL;
    base->dbFactories = NULL;
    base->eventFactories = NULL;
    base->guidProviders = NULL;
    
//    base->sysProvider = sysProvider;
#ifdef OCR_ENABLE_STATISTICS
    base->statsObject = statsObject;
#endif
    base->costFunction = costFunction;

    base->destruct = xePolicyDomainDestruct;
    base->start = xePolicyDomainStart;
    base->stop = xePolicyDomainStop;
    base->finish = xePolicyDomainFinish;
    base->processMessage = xePolicyDomainProcessMessage;
//    base->sendMessage = xePolicyDomainSendMessage;
//    base->receiveMessage = xePolicyDomainReceiveMessage;
//    base->getSys = xePolicyDomainGetSys;
#ifdef OCR_ENABLE_STATISTICS
    base->getStats = xeGetStats;
#endif

    // no inter-policy domain for simple XE
    base->neighbors = NULL;
    base->neighborCount = 0;

    //TODO populated by ini file factories. Need setters or something ?
    base->schedulers = NULL;
    base->allocators = NULL;
    
//    base->guid = UNINITIALIZED_GUID;
    return base;
}

static void destructPolicyDomainFactoryXe(ocrPolicyDomainFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrPolicyDomainFactory_t * newPolicyDomainFactoryXe(ocrParamList_t *perType) {
    ocrPolicyDomainFactoryXe_t* derived = (ocrPolicyDomainFactoryXe_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryXe_t), NULL);
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    
    ASSERT(base); // Check allocation
    
    base->instantiate = newPolicyDomainXe;
    base->destruct =  destructPolicyDomainFactoryXe;
    return base;
}

#endif /* ENABLE_POLICY_DOMAIN_XE */
