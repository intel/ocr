/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_DIST_POLICY_H__
#define __HC_DIST_POLICY_H__

#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_HC_DIST

#include "ocr-policy-domain.h"
#include "utils/ocr-utils.h"
#include "utils/hashtable.h"
#include "policy-domain/hc/hc-policy.h"

/******************************************************/
/* OCR-HC DISTRIBUTED POLICY DOMAIN                   */
/******************************************************/

typedef struct {
    ocrPolicyDomainHc_t base;
    u32 proxyLock; /**< Lock for creating proxies for remote DB */
    u8 (*baseProcessMessage)(struct _ocrPolicyDomain_t *self, struct _ocrPolicyMsg_t *msg,
                         u8 isBlocking);
    void (*baseStart)(struct _ocrPolicyDomain_t *self);
    void (*baseStop)(struct _ocrPolicyDomain_t *self);
} ocrPolicyDomainHcDist_t;

typedef struct {
    ocrPolicyDomainFactory_t base;
    void (*baseInitialize) (struct _ocrPolicyDomainFactory_t *factory, struct _ocrPolicyDomain_t* self,
#ifdef OCR_ENABLE_STATISTICS
         ocrStats_t *statsObject,
#endif
         ocrCost_t *costFunction, ocrParamList_t *perInstance);
    u8 (*baseProcessMessage)(struct _ocrPolicyDomain_t *self, struct _ocrPolicyMsg_t *msg,
                         u8 isBlocking);
    void (*baseStart)(struct _ocrPolicyDomain_t *self);
    void (*baseStop)(struct _ocrPolicyDomain_t *self);
} ocrPolicyDomainFactoryHcDist_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryHcDist(ocrParamList_t *perType);

#endif /* ENABLE_POLICY_DOMAIN_HC_DIST */
#endif /* __HC_DIST_POLICY_H__ */
