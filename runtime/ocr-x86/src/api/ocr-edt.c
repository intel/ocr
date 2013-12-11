/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-edt.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime.h"

u8 ocrEventCreate(ocrGuid_t *guid, ocrEventTypes_t eventType, bool takesArg) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t * pd;
    getCurrentEnv(&pd, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
    msg.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = *guid;
    PD_MSG_FIELD(properties) = takesArg;
    PD_MSG_FIELD(type) = eventType;
    pd->processMessage(pd, &msg, true);
    
    *guid = PD_MSG_FIELD(guid.guid);
    
#undef PD_MSG
#undef PD_TYPE
    return 0;
}

u8 ocrEventDestroy(ocrGuid_t eventGuid) {
    ocrEvent_t * event = NULL;
    ocrPolicyDomain_t *pd;
    getCurrentEnv(&pd, NULL);
    deguidify(pd, eventGuid, (u64*)&event, NULL);
    event->fctPtrs->destruct(event);
    return 0;
}

u8 ocrEventSatisfySlot(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*= INVALID_GUID*/, u32 slot) {
    ASSERT(eventGuid != NULL_GUID);
    ocrEvent_t * event = NULL;
    ocrPolicyDomain_t *pd = NULL;
    ocrFatGuid_t dataFGuid = {.guid = dataGuid, .metaDataPtr = NULL};
    getCurrentEnv(&pd, NULL, NULL);
    deguidify(pd, eventGuid, (u64*)&event, NULL);
    event->fctPtrs->satisfy(event, dataFGuid, slot);
    return 0;
}

u8 ocrEventSatisfy(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*= INVALID_GUID*/) {
    return ocrEventSatisfySlot(eventGuid, dataGuid, 0);
}

u8 ocrEdtTemplateCreate_internal(ocrGuid_t *guid, ocrEdt_t funcPtr, u32 paramc, u32 depc, const char* funcName) {
#ifdef OCR_ENABLE_EDT_NAMING
    ASSERT(funcName);
#endif
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, &msg, NULL);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EDTTEMP_CREATE
    msg.type = PD_MSG_EDTTEMP_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = *guid;
    PD_MSG_FIELD(funcPtr) = funcPtr;
    PD_MSG_FIELD(paramc) = paramc;
    PD_MSG_FIELD(depc) = depc;
    PD_MSG_FIELD(funcName) = funcName;

    if(pd->processMessage(pd, &msg, true) == 0) {
        *guid = PD_MSG_FIELD(guid.guid);
        return 0;
    }
    return 1;
#undef PD_MSG
#undef PD_TYPE
}

u8 ocrEdtTemplateDestroy(ocrGuid_t guid) {
    ocrPolicyDomain_t * pd = NULL;
    ocrTaskTemplate_t * taskTemplate = NULL;
    getCurrentEnv(&pd, NULL, NULL);
    deguidify(pd, guid, (u64*)&taskTemplate, NULL);
    taskTemplate->fctPtrs->destruct(taskTemplate);
    return 0;
}

u8 ocrEdtCreate(ocrGuid_t* edtGuid, ocrGuid_t templateGuid,
                u32 paramc, u64* paramv, u32 depc, ocrGuid_t *depv,
                u16 properties, ocrGuid_t affinity, ocrGuid_t *outputEvent) {

    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, &msg, NULL);
    
    ocrTaskTemplate_t *taskTemplate = NULL;
    
    deguidify(pd, templateGuid, (u64*)&taskTemplate, NULL);
    
    ASSERT(((taskTemplate->paramc == EDT_PARAM_UNK) && paramc != EDT_PARAM_DEF) ||
           (taskTemplate->paramc != EDT_PARAM_UNK && (paramc == EDT_PARAM_DEF || taskTemplate->paramc == paramc)));
    ASSERT(((taskTemplate->depc == EDT_PARAM_UNK) && depc != EDT_PARAM_DEF) ||
           (taskTemplate->depc != EDT_PARAM_UNK && (depc == EDT_PARAM_DEF || taskTemplate->depc == depc)));

    if(paramc == EDT_PARAM_DEF) {
        paramc = taskTemplate->paramc;
    }
    if(depc == EDT_PARAM_DEF) {
        depc = taskTemplate->depc;
    }

    // If paramc are expected, double check paramv is not NULL
    ASSERT((paramc > 0) ? (paramv != NULL) : true);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_WORK_CREATE
    msg.type = PD_MSG_WORK_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = *edtGuid;
    PD_MSG_FIELD(templateGuid.guid) = templateGuid;
    PD_MSG_FIELD(templateGuid.metaDataPtr) = (void*)taskTemplate;
    PD_MSG_FIELD(affinity.guid) = affinity;
    PD_MSG_FIELD(outputEvent.guid) = *outputEvent;
    PD_MSG_FIELD(paramv) = paramv;
    PD_MSG_FIELD(paramc) = paramc;
    PD_MSG_FIELD(depc) = depc;
    PD_MSG_FIELD(properties) = properties;
    PD_MSG_FIELD(workType) = EDT_WORKTYPE;

    if(pd->processMessage(pd, &msg, true) != 0)
        return 1; // Some error code
    
    *edtGuid = PD_MSG_FIELD(guid.guid);
    *outputEvent = PD_MSG_FIELD(outputEvent.guid);
    
    // If guids dependences were provided, add them now
    // TODO: This should probably just send the messages
    // to the PD
    if(depv != NULL) {
        ASSERT(depc != 0);
        u32 i = 0;
        while(i < depc) {
            ocrAddDependence(depv[i], *edtGuid, i, DB_DEFAULT_MODE);
            ++i;
        }
    }
    return 0;
}

u8 ocrEdtDestroy(ocrGuid_t edtGuid) {
    ocrPolicyDomain_t * pd = NULL;
    ocrTask_t * task = NULL;
    getCurrentEnv(&pd, NULL, NULL);
    deguidify(pd, edtGuid, (u64*)&task, NULL);
    task->fctPtrs->destruct(task);
    return 0;
}

// TODO: Pass down the mode information!!
u8 ocrAddDependence(ocrGuid_t source, ocrGuid_t destination, u32 slot,
                    ocrDbAccessMode_t mode) {
    // TODO: Register dependence is not a configurable function
    // FIXME!!
    registerDependence(source, destination, slot);
    return 0;
}
