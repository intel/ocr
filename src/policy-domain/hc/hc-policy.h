/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_POLICY_H__
#define __HC_POLICY_H__

#include "ocr-policy-domain.h"
#include "ocr-utils.h"

typedef struct _ocrPolicyCtxHc_t {
    ocrPolicyCtx_t base;
} ocrPolicyCtxHc_t;

typedef struct _ocrPolicyCtxFactoryHc_t {
    ocrPolicyCtxFactory_t base;
} ocrPolicyCtxFactoryHc_t;

/******************************************************/
/* OCR-HC POLICY DOMAIN                               */
/******************************************************/

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryHc_t;

typedef struct {
    ocrPolicyDomain_t base;
} ocrPolicyDomainHc_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryHc(ocrParamList_t *perType);
ocrPolicyCtxFactory_t *newPolicyCtxFactoryHc(ocrParamList_t *perType);
#endif /* __HC_POLICY_H__ */
