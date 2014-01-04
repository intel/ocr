/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __HC_POLICY_H__
#define __HC_POLICY_H__

#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_HC

#include "ocr-policy-domain.h"
#include "utils/ocr-utils.h"

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

#endif /* ENABLE_POLICY_DOMAIN_HC */
#endif /* __HC_POLICY_H__ */
