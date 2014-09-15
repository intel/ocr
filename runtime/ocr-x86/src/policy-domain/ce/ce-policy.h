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

#define CE_TAKE_CHUNK_SIZE 1        //number of tasks that one CE "takes" from another CE

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryCe_t;

typedef struct {
    ocrPolicyDomain_t base;
    u8 xeCount;                     //number of XE's serviced by this CE
    u32 shutdownCount;              //dynamic counter to keep track of agents alreadt shutdown
    u32 shutdownMax;                //maximum number of agents participating in the shutdown process
    bool shutdownMode;              //flag indicating shutdown mode
    bool *ceCommTakeActive;         //flag to coordinate the search for work
} ocrPolicyDomainCe_t;

typedef struct {
    paramListPolicyDomainInst_t base;
    u32 xeCount;
    u32 neighborCount;
} paramListPolicyDomainCeInst_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryCe(ocrParamList_t *perType);

#endif /* ENABLE_POLICY_DOMAIN_CE */
#endif /* __CE_POLICY_H__ */
