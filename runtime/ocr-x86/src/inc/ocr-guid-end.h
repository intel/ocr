/**
 * @brief OCR internal API to GUID management
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_GUID_END_H__
#define __OCR_GUID_END_H__

#if !defined(__GUID_END_MARKER__)
#error "Do not include ocr-guid-end.h yourself"
#endif

#include "ocr-event.h"
#include "ocr-policy-domain.h"


static inline u8 guidKind(struct _ocrPolicyDomain_t * pd, ocrFatGuid_t guid,
                          ocrGuidKind* kindRes) {

    u8 returnCode = 0;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_INFO

    msg.type = PD_MSG_GUID_INFO | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid) = guid;
    PD_MSG_FIELD(properties) = KIND_GUIDPROP;
    returnCode = pd->processMessage(pd, &msg, true);

    if(returnCode == 0)
        *kindRes = PD_MSG_FIELD(kind);

    return returnCode;
#undef PD_MSG
#undef PD_TYPE
}


static inline u8 guidify(struct _ocrPolicyDomain_t * pd, u64 val,
                         ocrFatGuid_t * guidRes, ocrGuidKind kind) {
    u8 returnCode = 0;
    ocrPolicyMsg_t msg;
    getCurrentEnv(&pd, NULL, NULL, &msg);
    
    ASSERT(guidRes->guid == NULL_GUID || guidRes->guid == UNINITIALIZED_GUID);
    
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_CREATE

    msg.type = PD_MSG_GUID_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.metaDataPtr) = (void*)val;
    PD_MSG_FIELD(guid.guid) = NULL_GUID;
    PD_MSG_FIELD(size) = 0;
    PD_MSG_FIELD(kind) = kind;
    returnCode = pd->processMessage(pd, &msg, true);

    if(returnCode == 0) {
        *guidRes = PD_MSG_FIELD(guid);
        ASSERT((u64)(guidRes->metaDataPtr) == val);
    }
    return returnCode;
#undef PD_MSG
#undef PD_TYPE
}

static inline u8 deguidify(struct _ocrPolicyDomain_t * pd, ocrFatGuid_t *res,
                           ocrGuidKind* kindRes) {

    u32 properties = 0;
    u8 returnCode = 0;
    if(kindRes)
        properties |= KIND_GUIDPROP;
    if(res->metaDataPtr == NULL)
        properties |= RMETA_GUIDPROP;
    
    if(properties) {
        ocrPolicyMsg_t msg;
        getCurrentEnv(&pd, NULL, NULL, &msg);
#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_INFO
        msg.type = PD_MSG_GUID_INFO | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
        PD_MSG_FIELD(guid) = *res;
        PD_MSG_FIELD(properties) = properties;
        returnCode = pd->processMessage(pd, &msg, true);

        if(returnCode) {
            res->metaDataPtr = NULL;
            return returnCode;
        }
        
        if(!(res->metaDataPtr)) {
            res->metaDataPtr = PD_MSG_FIELD(guid.metaDataPtr);
        }
        if(kindRes) {
            *kindRes = PD_MSG_FIELD(kind);
        }
#undef PD_MSG
#undef PD_TYPE
        return returnCode;
    }
    return 0;
}

static inline bool isDatablockGuid(ocrPolicyDomain_t *pd, ocrFatGuid_t guid) {
    if (NULL_GUID == guid.guid) {
        return false;
    }
    
    ocrGuidKind kind = OCR_GUID_NONE;
    if(guidKind(pd, guid, &kind) == 0)
        return kind == OCR_GUID_DB;
    return false;
}

static inline bool isEventGuid(ocrPolicyDomain_t *pd, ocrFatGuid_t guid) {
    if (NULL_GUID == guid.guid) {
        return false;
    }
    
    ocrGuidKind kind = OCR_GUID_NONE;
    if(guidKind(pd, guid, &kind) == 0)
        return kind == OCR_GUID_EVENT;
    return false;
}

static inline bool isEdtGuid(ocrPolicyDomain_t *pd, ocrFatGuid_t guid) {
    if (NULL_GUID == guid.guid) {
        return false;
    }
    
    ocrGuidKind kind = OCR_GUID_NONE;
    if(guidKind(pd, guid, &kind) == 0)
        return kind == OCR_GUID_EDT;
    return false;
}

static inline ocrEventTypes_t eventType(ocrPolicyDomain_t *pd, ocrFatGuid_t guid) {

    if(!guid.metaDataPtr) {
        RESULT_ASSERT(deguidify(pd, &guid, NULL), ==, 0);
    }
    // We now have a R/O copy of the event
    return (((ocrEvent_t*)guid.metaDataPtr)->kind);
}

#endif /* __OCR_GUID_END_H__ */
