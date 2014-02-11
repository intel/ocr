/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __CE_POLICY_H__
#define __CE_POLICY_H__

#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_CE

#include "ocr-policy-domain.h"
#include "utils/ocr-utils.h"

/******************************************************/
/* OCR-CE POLICY DOMAIN                               */
/******************************************************/

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryCe_t;

typedef struct {
    ocrPolicyDomain_t base;
    u64 shutdownCount;
    ocrPolicyMsg_t * shutdownMsgs;
} ocrPolicyDomainCe_t;

typedef struct {
    paramListPolicyDomainInst_t base;
    u32 neighborCount;
} paramListPolicyDomainCeInst_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryCe(ocrParamList_t *perType);

#endif /* ENABLE_POLICY_DOMAIN_CE */
#endif /* __CE_POLICY_H__ */
