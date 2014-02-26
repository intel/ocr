
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

// In all these functions, we consider only a single PD. In other words, in XE, we
// deal with everything locally and never send messages


static u8 xeCreateEdt(ocrPolicyDomain_t *self, ocrFatGuid_t *guid,
                      ocrFatGuid_t  edtTemplate, u32 *paramc, u64* paramv,
                      u32 *depc, u32 properties, ocrFatGuid_t affinity,
                      ocrFatGuid_t * outputEvent) {

    
    ocrTaskTemplate_t *taskTemplate = (ocrTaskTemplate_t*)edtTemplate.metaDataPtr;
    
    ASSERT(((taskTemplate->paramc == EDT_PARAM_UNK) && *paramc != EDT_PARAM_DEF) ||
           (taskTemplate->paramc != EDT_PARAM_UNK && (*paramc == EDT_PARAM_DEF ||
                                                      taskTemplate->paramc == *paramc)));
    ASSERT(((taskTemplate->depc == EDT_PARAM_UNK) && *depc != EDT_PARAM_DEF) ||
           (taskTemplate->depc != EDT_PARAM_UNK && (*depc == EDT_PARAM_DEF ||
                                                    taskTemplate->depc == *depc)));

    if(*paramc == EDT_PARAM_DEF) {
        *paramc = taskTemplate->paramc;
    }
    if(*depc == EDT_PARAM_DEF) {
        *depc = taskTemplate->depc;
    }
    // If paramc are expected, double check paramv is not NULL
    if((*paramc > 0) && (paramv == NULL)) {
        ASSERT(0);
        return OCR_EINVAL;
    }

    ocrTask_t * base = self->taskFactories[0]->instantiate(
        self->taskFactories[0], edtTemplate, *paramc, paramv,
        *depc, properties, affinity, outputEvent, NULL);
    
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

static void convertDepAddToSatisfy(ocrPolicyDomain_t *self, ocrFatGuid_t dbGuid,
                                   ocrFatGuid_t destGuid, u32 slot) {

    ocrPolicyMsg_t msg;
    getCurrentEnv(NULL, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_SATISFY
    msg.type = PD_MSG_DEP_SATISFY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid) = destGuid;
    PD_MSG_FIELD(payload) = dbGuid;
    PD_MSG_FIELD(slot) = slot;
    PD_MSG_FIELD(properties) = 0;
    RESULT_ASSERT(self->processMessage(self, &msg, false), ==, 0);
#undef PD_MSG
#undef PD_TYPE
}
#ifdef OCR_ENABLE_STATISTICS
static ocrStats_t* xeGetStats(ocrPolicyDomain_t *self) {
    return self->statsObject;
}
#endif

static u8 xeProcessCeRequest(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg) {
    u8 returnCode = 0;
    msg->srcLocation  = self->myLocation;
    msg->destLocation = self->parentLocation;
    u32 type = (msg->type & PD_MSG_TYPE_ONLY);
    RESULT_ASSERT(self->workers[0]->fcts.sendMessage(self->workers[0], self->parentLocation, &msg), ==, 0);
    if (msg->type & PD_MSG_REQ_RESPONSE) {
        RESULT_ASSERT(self->workers[0]->fcts.waitMessage(self->workers[0], &msg), ==, 0);
        if (msg->srcLocation == self->myLocation) {
            ASSERT((msg->type & PD_MSG_TYPE_ONLY) == type);
            ASSERT((!(msg->type & PD_MSG_REQUEST)) && (msg->type & PD_MSG_RESPONSE));
        } else {
            RESULT_ASSERT(self->processMessage(self, msg, false), ==, 0);
            returnCode = OCR_EINTR;
        }
    }
    return returnCode;
}

static u8 xeProcessCeResponse(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg) {
    ASSERT(msg->type & PD_MSG_REQ_RESPONSE);
    msg->type &= ~PD_MSG_REQUEST;
    msg->type |=  PD_MSG_RESPONSE;
    RESULT_ASSERT(self->workers[0]->fcts.sendMessage(self->workers[0], msg->srcLocation, &msg), ==, 0);
    return 0;
}

u8 xePolicyDomainProcessMessage(ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg, u8 isBlocking) {

    u8 returnCode = 0;
    ASSERT((msg->type & PD_MSG_REQUEST) && (!(msg->type & PD_MSG_RESPONSE))); 
    switch(msg->type & PD_MSG_TYPE_ONLY) {
    case PD_MSG_DB_CREATE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DB_CREATE
        // TODO: Add properties whether DB needs to be acquired or not
        // This would impact where we do the PD_MSG_MEM_ALLOC for example
        // For now we deal with both USER and RT dbs the same way
        ASSERT(PD_MSG_FIELD(dbType) == USER_DBTYPE || PD_MSG_FIELD(dbType) == RUNTIME_DBTYPE);
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        returnCode = xeProcessCeRequest(self, msg);
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
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        returnCode = xeProcessCeRequest(self, msg);
#undef PD_MSG
#undef PD_TYPE
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
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        returnCode = xeProcessCeRequest(self, msg);
#undef PD_MSG
#undef PD_TYPE
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
        ASSERT(db->fctId == self->dbFactories[0]->factoryId);
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        returnCode = xeProcessCeRequest(self, msg);
#undef PD_MSG
#undef PD_TYPE
        break;
    }

    case PD_MSG_MEM_ALLOC:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_ALLOC
        msg->type -= PD_MSG_MEM_ALLOC;            // Help CE differentiate between XE-sent messages
        msg->type += PD_MSG_MEM_ALLOC_FOR_CLIENT; // and those of its own making.
        // ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);  TODO:  I don't think this assert holds up any more, now that the request is being forwarded to the CE. BRN
        ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);  // TODO: On second thought, maybe it is still applicable.  Experiment.  BRN 29 Jan 2014
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        ASSERT (ocrLocation_getEngineIndex(self->myLocation) >= 1);
        returnCode = xeProcessCeRequest(self, msg);
#undef PD_MSG
#undef PD_TYPE
        break;
    }

    case PD_MSG_MEM_UNALLOC:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_MEM_UNALLOC
        msg->type -= PD_MSG_MEM_UNALLOC;            // Help CE differentiate between XE-sent messages
        msg->type += PD_MSG_MEM_UNALLOC_FOR_CLIENT; // and those of its own making.
        // ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);  TODO:  I don't think this assert holds up any more, now that the request is being forwarded to the CE.
        ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
        returnCode = xeProcessCeRequest(self, msg);
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
            &PD_MSG_FIELD(paramc), PD_MSG_FIELD(paramv), &PD_MSG_FIELD(depc),
            PD_MSG_FIELD(properties), PD_MSG_FIELD(affinity), outputEvent);
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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
                                 
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }
        
    case PD_MSG_COMM_TAKE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_TAKE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        returnCode = xeProcessCeRequest(self, msg);
        if (returnCode == 0) {
            PD_MSG_FIELD(properties) = 0;
            localDeguidify(self, (PD_MSG_FIELD(guids)));
            // For now, we return the execute function for EDTs
            PD_MSG_FIELD(extra) = (u64)(self->taskFactories[0]->fcts.execute);
        }
#undef PD_MSG
#undef PD_TYPE
        break;
    }
        
    case PD_MSG_COMM_GIVE:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_COMM_GIVE
        ASSERT(PD_MSG_FIELD(type) == OCR_GUID_EDT);
        returnCode = xeProcessCeRequest(self, msg);
#undef PD_MSG
#undef PD_TYPE
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
        if(srcKind == OCR_GUID_DB) {
            // This is equivalent to an immediate satisfy
            convertDepAddToSatisfy(self, src, dest, PD_MSG_FIELD(slot));
        } else {
            if(srcKind == OCR_GUID_EVENT) {
                ocrEvent_t *evt = (ocrEvent_t*)(src.metaDataPtr);
                ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
                self->eventFactories[0]->fcts[evt->kind].registerWaiter(
                    evt, dest, PD_MSG_FIELD(slot), true);
            } else {
                // Some sanity check
                ASSERT(srcKind == OCR_GUID_EDT);
            }
            if(dstKind == OCR_GUID_EDT) {
                ocrTask_t *task = (ocrTask_t*)(dest.metaDataPtr);
                ASSERT(task->fctId == self->taskFactories[0]->factoryId);
                self->taskFactories[0]->fcts.registerSignaler(task, src, PD_MSG_FIELD(slot), true);
            } else if(dstKind == OCR_GUID_EVENT) {
                ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
                ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
                self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                    evt, src, PD_MSG_FIELD(slot), true);
            } else {
                ASSERT(0); // Cannot have other types of destinations
            }
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
    statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }

    case PD_MSG_DEP_REGSIGNALER:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_REGSIGNALER
        // We first get information about the signaler and destination
        ocrGuidKind signalerKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(signaler.guid),
            (u64*)(&(PD_MSG_FIELD(signaler.metaDataPtr))), &signalerKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);
        
        ocrFatGuid_t signaler = PD_MSG_FIELD(signaler);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);
        
        switch(dstKind) {
        case OCR_GUID_EVENT:
        {
            ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            self->eventFactories[0]->fcts[evt->kind].registerSignaler(
                evt, signaler, PD_MSG_FIELD(slot), false);
            break;
        }
        case OCR_GUID_EDT:
        {
            ocrTask_t *edt = (ocrTask_t*)(dest.metaDataPtr);
            ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
            self->taskFactories[0]->fcts.registerSignaler(
                edt, signaler, PD_MSG_FIELD(slot), false);
            break;
        }
        default:
            ASSERT(0); // No other things we can register signalers on
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }

    case PD_MSG_DEP_REGWAITER:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_REGWAITER        
// We first get information about the signaler and destination
        ocrGuidKind waiterKind, dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(waiter.guid),
            (u64*)(&(PD_MSG_FIELD(waiter.metaDataPtr))), &waiterKind);
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(dest.guid),
            (u64*)(&(PD_MSG_FIELD(dest.metaDataPtr))), &dstKind);
        
        ocrFatGuid_t waiter = PD_MSG_FIELD(waiter);
        ocrFatGuid_t dest = PD_MSG_FIELD(dest);

        ASSERT(dstKind == OCR_GUID_EVENT); // Waiters can only wait on events
        ocrEvent_t *evt = (ocrEvent_t*)(dest.metaDataPtr);
        ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
        self->eventFactories[0]->fcts[evt->kind].registerWaiter(
            evt, waiter, PD_MSG_FIELD(slot), false);
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }

    case PD_MSG_DEP_SATISFY:
    {
#define PD_MSG msg
#define PD_TYPE PD_MSG_DEP_SATISFY
        ocrGuidKind dstKind;
        self->guidProviders[0]->fcts.getVal(
            self->guidProviders[0], PD_MSG_FIELD(guid.guid),
            (u64*)(&(PD_MSG_FIELD(guid.metaDataPtr))), &dstKind);

        ocrFatGuid_t dst = PD_MSG_FIELD(guid);
        if(dstKind == OCR_GUID_EVENT) {
            ocrEvent_t *evt = (ocrEvent_t*)(dst.metaDataPtr);
            ASSERT(evt->fctId == self->eventFactories[0]->factoryId);
            self->eventFactories[0]->fcts[evt->kind].satisfy(
                evt, PD_MSG_FIELD(payload), PD_MSG_FIELD(slot));
        } else {
            if(dstKind == OCR_GUID_EDT) {
                ocrTask_t *edt = (ocrTask_t*)(dst.metaDataPtr);
                ASSERT(edt->fctId == self->taskFactories[0]->factoryId);
                self->taskFactories[0]->fcts.satisfy(
                    edt, PD_MSG_FIELD(payload), PD_MSG_FIELD(slot));
            } else {
                ASSERT(0); // We can't satisfy anything else
            }
        }
#ifdef OCR_ENABLE_STATISTICS
        // TODO: Fixme
        statsDEP_ADD(pd, getCurrentEDT(), NULL, signalerGuid, waiterGuid, NULL, slot);
#endif
#undef PD_MSG
#undef PD_TYPE
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;            
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
        ASSERT(0);
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }
        
    case PD_MSG_SAL_READ:
    {
        ASSERT(0);
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }
    
    case PD_MSG_SAL_WRITE:
    {
        ASSERT(0);
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
        break;
    }
        
    case PD_MSG_MGT_SHUTDOWN:
    {
        self->stop(self);
        if (msg->srcLocation == self->myLocation) {
            returnCode = xeProcessCeRequest(self, msg);
        } else {
            returnCode = xeProcessCeResponse(self, msg);
        }
        break;
    }
        
    case PD_MSG_MGT_FINISH:
    {
        self->finish(self);
        msg->type &= ~PD_MSG_REQUEST;
        msg->type |=  PD_MSG_RESPONSE;
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

    return returnCode;
}

void* xePdMalloc(ocrPolicyDomain_t *self, u64 size) {
    // Just try in the first allocator
    //return self->allocators[0]->fcts.allocate(self->allocators[0], size);
    void *ptr;
    ocrPolicyMsg_t msg;
    ocrPolicyMsg_t* pmsg = &msg;
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_MEM_ALLOC
    PD_MSG_FIELD(type) = DB_MEMTYPE;
    PD_MSG_FIELD(size) = size;
    msg.type = PD_MSG_MEM_ALLOC_FOR_CLIENT  | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE; // and those of its own making.
    // ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);  TODO:  I don't think this assert holds up any more, now that the request is being forwarded to the CE. BRN
//    ASSERT(PD_MSG_FIELD(allocatingPD.guid) == self->fguid.guid);  // TODO: On second thought, maybe it is still applicable.  Experiment.  BRN 29 Jan 2014
    ASSERT(self->workerCount == 1);              // Assert this XE has exactly one worker.
    u8 msgResult = xeProcessCeRequest(self, pmsg);
    ASSERT (msgResult == 0);   // TODO: Are there error cases I need to handle?  How?
    ptr = pmsg->args.PD_MSG_STRUCT_NAME(PD_MSG_MEM_ALLOC).ptr;
#undef PD_TYPE
#undef PD_MSG
    return ptr;
}

void xePdFree(ocrPolicyDomain_t *self, void* addr) {
    return self->allocators[0]->fcts.free(self->allocators[0], addr);
}

ocrPolicyDomain_t * newPolicyDomainXe(ocrPolicyDomainFactory_t * policy,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance) {

    ocrPolicyDomainXe_t * derived = (ocrPolicyDomainXe_t *) runtimeChunkAlloc(sizeof(ocrPolicyDomainXe_t), NULL);
    ocrPolicyDomain_t * base = (ocrPolicyDomain_t *) derived;

    ASSERT(base);
//    base->sysProvider = sysProvider;
#ifdef OCR_ENABLE_STATISTICS
    base->statsObject = statsObject;
#endif
    base->costFunction = costFunction;

    base->destruct = FUNC_ADDR(void (*)(ocrPolicyDomain_t *), xePolicyDomainDestruct);
    base->begin = FUNC_ADDR(void (*)(ocrPolicyDomain_t *), xePolicyDomainBegin);
    base->start = FUNC_ADDR(void (*)(ocrPolicyDomain_t *), xePolicyDomainStart);
    base->stop = FUNC_ADDR(void (*)(ocrPolicyDomain_t *), xePolicyDomainStop);
    base->finish = FUNC_ADDR(void (*)(ocrPolicyDomain_t *), xePolicyDomainFinish);
    base->processMessage = FUNC_ADDR(u8 (*)(ocrPolicyDomain_t *, ocrPolicyMsg_t *msg, u8 isBlocking), xePolicyDomainProcessMessage);
    base->pdMalloc = FUNC_ADDR(void* (*)(ocrPolicyDomain_t *, u64), xePdMalloc);
    base->pdFree = FUNC_ADDR(void (*)(ocrPolicyDomain_t *, void*), xePdFree);

#ifdef OCR_ENABLE_STATISTICS
    base->getStats = xeGetStats;
#endif

    base->myLocation = ((paramListPolicyDomainInst_t*)perInstance)->location;

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
    ocrPolicyDomainFactoryXe_t* derived = (ocrPolicyDomainFactoryXe_t*) runtimeChunkAlloc(sizeof(ocrPolicyDomainFactoryXe_t), (void *)1);
    ocrPolicyDomainFactory_t* base = (ocrPolicyDomainFactory_t*) derived;
    
    ASSERT(base); // Check allocation
    
    base->instantiate = newPolicyDomainXe;
    base->destruct =  destructPolicyDomainFactoryXe;
    return base;
}

#endif /* ENABLE_POLICY_DOMAIN_XE */
