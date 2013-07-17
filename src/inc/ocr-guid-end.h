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
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"

static inline u8 guidKind(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid,
                          ocrGuidKind* kindRes) {
    u64 ptrRes;
    return pd->getInfoForGuid(pd, guid, &ptrRes, kindRes, NULL);
}

static inline u8 guidify(struct _ocrPolicyDomain_t * pd, u64 ptr, ocrGuid_t * guidRes,
                         ocrGuidKind kind) {

    return pd->getGuid(pd, guidRes, ptr, kind, NULL);
}

static inline u8 deguidify(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid, u64* ptrRes,
                           ocrGuidKind* kindRes) {
    return pd->getInfoForGuid(pd, guid, ptrRes, kindRes, NULL);
}

static inline bool isDatablockGuid(ocrGuid_t guid) {
    if (NULL_GUID == guid) {
        return false;
    }
    ocrGuidKind kind;
    guidKind(getCurrentPD(),guid, &kind);
    return kind == OCR_GUID_DB;
}

static inline bool isEventGuid(ocrGuid_t guid) {
    if (NULL_GUID == guid) {
        return false;
    }
    ocrGuidKind kind;
    guidKind(getCurrentPD(),guid, &kind);
    return kind == OCR_GUID_EVENT;
}

static inline bool isEdtGuid(ocrGuid_t guid) {
    if (NULL_GUID == guid) {
        return false;
    }
    ocrGuidKind kind;
    guidKind(getCurrentPD(),guid, &kind);
    return kind == OCR_GUID_EDT;
}

static inline bool isEventLatchGuid(ocrGuid_t guid) {
    if(isEventGuid(guid)) {
        ocrEvent_t * event = NULL;
        deguidify(getCurrentPD(), guid, (u64*)&event, NULL);
        return (event->kind == OCR_EVENT_LATCH_T);
    }
    return false;
}

static inline bool isEventSingleGuid(ocrGuid_t guid) {
    if(isEventGuid(guid)) {
        ocrEvent_t * event = NULL;
        deguidify(getCurrentPD(), guid, (u64*)&event, NULL);
        return ((event->kind == OCR_EVENT_ONCE_T)
                || (event->kind == OCR_EVENT_IDEM_T)
                || (event->kind == OCR_EVENT_STICKY_T));
    }
    return false;
}

static inline bool isEventGuidOfKind(ocrGuid_t guid, ocrEventTypes_t eventKind) {
    if (isEventGuid(guid)) {
        ocrEvent_t * event = NULL;
        deguidify(getCurrentPD(), guid, (u64*)&event, NULL);
        return event->kind == eventKind;
    }
    return false;
}

#endif /* __OCR_GUID_END_H__ */
