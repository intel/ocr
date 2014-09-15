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
    u32 rank;           // For MPI use
    volatile u32 state; // State of the policy domain
    // Bottom 4 bits indicate state:
    //  - 0000b: Unitialized
    //  - 0001b: PD usable (up and running)
    //  - 0010b: Request to enter stop (no more users can use the PD)
    //  - 0110b: Stop entered. Users have stopped using the PD
    //  - 1010b: Stop completed successfully, can 'finish' the PD
    //  - 1110b: Policy domain fully shut down
    // The upper bits count the number of users ('readers')
} ocrPolicyDomainHc_t;

typedef struct {
    paramListPolicyDomainInst_t base;
    u32 rank;
} paramListPolicyDomainHcInst_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryHc(ocrParamList_t *perType);

ocrPolicyDomain_t * newPolicyDomainHc(ocrPolicyDomainFactory_t * policy,
#ifdef OCR_ENABLE_STATISTICS
                                      ocrStats_t *statsObject,
#endif
                                      ocrCost_t *costFunction, ocrParamList_t *perInstance);

#endif /* ENABLE_POLICY_DOMAIN_HC */
#endif /* __HC_POLICY_H__ */
