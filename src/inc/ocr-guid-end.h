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

/*! \brief Resolve the kind of a guid (could be an event, an edt, etc...)
 *  \param[in] pd          Policy domain
 *  \param[in] guid        The gid for which we want the kind
 *  \param[out] kindRes    Parameter-result to contain the kind
 */
static inline u8 guidKind(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid,
                          ocrGuidKind* kindRes) {
    u64 ptrRes;
    return pd->getInfoForGuid(pd, guid, &ptrRes, kindRes, NULL);
}

/*! \brief Get the kind of a guid
 *  \param[in] pd          Policy domain
 *  \param[in] ptr         The pointer for which we want a guid
 *  \param[out] guidRes    Parameter-result to contain the guid
 *  \param[in] kind        The kind of the guid (whether is an event, an edt, etc...)
 */
static inline u8 guidify(struct _ocrPolicyDomain_t * pd, u64 ptr, ocrGuid_t * guidRes,
                         ocrGuidKind kind) {

    return pd->getGuid(pd, guidRes, ptr, kind, NULL);
}

/*! \brief Resolve a pointer out of a guid
 *  \param[in] pd          Policy domain
 *  \param[in] guid        The guid we want to resolve
 *  \param[out] ptrRes     Parameter-result to contain the pointer
 *  \param[out] kindRes    Parameter-result to contain the kind
 */
static inline u8 deguidify(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid, u64* ptrRes,
                           ocrGuidKind* kindRes) {
    return pd->getInfoForGuid(pd, guid, ptrRes, kindRes, NULL);
}

/*! \brief Check if a guid represents a data-block
 *  \param[in] guid        The guid to check the kind
 */
static inline bool isDatablockGuid(ocrGuid_t guid) {
    if (NULL_GUID == guid) {
        return false;
    }
    ocrGuidKind kind;
    guidKind(getCurrentPD(),guid, &kind);
    return kind == OCR_GUID_DB;
}

/*! \brief Check if a guid represents an event
 *  \param[in] guid        The guid to check the kind
 */
static inline bool isEventGuid(ocrGuid_t guid) {
    if (NULL_GUID == guid) {
        return false;
    }
    ocrGuidKind kind;
    guidKind(getCurrentPD(),guid, &kind);
    return kind == OCR_GUID_EVENT;
}

/*! \brief Check if a guid represents an EDT
 *  \param[in] guid        The guid to check the kind
 */
static inline bool isEdtGuid(ocrGuid_t guid) {
    if (NULL_GUID == guid) {
        return false;
    }
    ocrGuidKind kind;
    guidKind(getCurrentPD(),guid, &kind);
    return kind == OCR_GUID_EDT;
}

/*! \brief Check if a guid represents a latch-event
 *  \param[in] guid        The guid to check the kind
 */
static inline bool isEventLatchGuid(ocrGuid_t guid) {
    if(isEventGuid(guid)) {
        ocrEvent_t * event = NULL;
        deguidify(getCurrentPD(), guid, (u64*)&event, NULL);
        return (event->kind == OCR_EVENT_LATCH_T);
    }
    return false;
}

/*! \brief Check if a guid represents a single event (once, idempotent or sticky)
 *  \param[in] guid        The guid to check the kind
 */
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

/*! \brief Check if a guid represents an event of a certain kind
 *  \param[in] guid        The guid to check the kind
 *  \param[in] eventKind   The event kind to check for
 *  @return true if the guid's kind matches 'eventKind'
 */
static inline bool isEventGuidOfKind(ocrGuid_t guid, ocrEventTypes_t eventKind) {
    if (isEventGuid(guid)) {
        ocrEvent_t * event = NULL;
        deguidify(getCurrentPD(), guid, (u64*)&event, NULL);
        return event->kind == eventKind;
    }
    return false;
}

#endif /* __OCR_GUID_END_H__ */
