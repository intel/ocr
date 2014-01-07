/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "debug.h"
#include "ocr-edt.h"
#include "ocr-policy-domain.h"
#include "ocr-runtime.h"

u8 ocrEventCreate(ocrGuid_t *guid, ocrEventTypes_t eventType, bool takesArg) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_CREATE
    msg.type = PD_MSG_EVT_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = *guid;
    PD_MSG_FIELD(properties) = takesArg;
    PD_MSG_FIELD(type) = eventType;
    if(pd->processMessage(pd, &msg, true) == 0) {
        *guid = PD_MSG_FIELD(guid.guid);
        return 0;
    }
    *guid = NULL_GUID;
#undef PD_MSG
#undef PD_TYPE
    return 1;
}

u8 ocrEventDestroy(ocrGuid_t eventGuid) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_DESTROY
    msg.type = PD_MSG_EVT_DESTROY | PD_MSG_REQUEST;
    
    PD_MSG_FIELD(guid.guid) = eventGuid;
    PD_MSG_FIELD(properties) = 0;
    return pd->processMessage(pd, &msg, false);
    
#undef PD_MSG
#undef PD_TYPE
}

u8 ocrEventSatisfySlot(ocrGuid_t eventGuid, ocrGuid_t dataGuid /*= INVALID_GUID*/, u32 slot) {

    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EVT_SATISFY
    msg.type = PD_MSG_EVT_SATISFY | PD_MSG_REQUEST;
    
    PD_MSG_FIELD(guid.guid) = eventGuid;
    PD_MSG_FIELD(payload.guid) = dataGuid;
    PD_MSG_FIELD(slot) = slot;
    PD_MSG_FIELD(properties) = 0;
    return pd->processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
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
    getCurrentEnv(&pd, NULL, NULL, &msg);
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
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_EDTTEMP_DESTROY
    msg.type = PD_MSG_EDTTEMP_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = guid;
    return pd->processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
}

u8 ocrEdtCreate(ocrGuid_t* edtGuid, ocrGuid_t templateGuid,
                u32 paramc, u64* paramv, u32 depc, ocrGuid_t *depv,
                u16 properties, ocrGuid_t affinity, ocrGuid_t *outputEvent) {

    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
    
    ocrTaskTemplate_t *taskTemplate = NULL;
    
    deguidify(pd, templateGuid, (u64*)&taskTemplate, NULL);
    
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
#undef PD_MSG
#undef PD_TYPE
}

u8 ocrEdtDestroy(ocrGuid_t edtGuid) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_WORK_DESTROY
    msg.type = PD_MSG_WORK_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD(guid.guid) = edtGuid;
    return pd->processMessage(pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
}

// TODO: Pass down the mode information!!
u8 ocrAddDependence(ocrGuid_t source, ocrGuid_t destination, u32 slot,
                    ocrDbAccessMode_t mode) {
    ocrPolicyMsg_t msg;
    ocrPolicyDomain_t *pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_DEP_ADD
    msg.type = PD_MSG_DEP_ADD | PD_MSG_REQUEST;
    PD_MSG_FIELD(source.guid) = source;
    PD_MSG_FIELD(dest.guid) = destination;
    PD_MSG_FIELD(slot) = slot;
    PD_MSG_FIELD(properties) = mode;
#undef PD_MSG
#undef PD_TYPE
    return pd->processMessage(pd, &msg, false);
}
