/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __XE_POLICY_H__
#define __XE_POLICY_H__

#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_XE

#include "ocr-policy-domain.h"
#include "utils/ocr-utils.h"

#define MAX_XE_TASK 1024

/******************************************************/
/* OCR-XE POLICY DOMAIN                               */
/******************************************************/

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryXe_t;

typedef struct {
    ocrPolicyDomain_t base;
    ocrFatGuid_t fguidPool[MAX_XE_TASK]; // Hacky task pool; needs to replaced by real task pool
    u32 taskCounter;
} ocrPolicyDomainXe_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryXe(ocrParamList_t *perType);

#endif /* ENABLE_POLICY_DOMAIN_XE */
#endif /* __XE_POLICY_H__ */
