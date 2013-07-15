/**
 * @brief OCR internal API to GUID management
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

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

#endif /* __OCR_GUID_END_H__ */
