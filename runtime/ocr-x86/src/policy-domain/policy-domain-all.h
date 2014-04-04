/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __POLICY_DOMAIN_ALL_H_
#define __POLICY_DOMAIN_ALL_H_

#include "debug.h"
#include "ocr-config.h"
#include "ocr-policy-domain.h"
#include "utils/ocr-utils.h"

typedef enum _policyDomainType_t {
    policyDomainHc_id,
    policyDomainXe_id,
    policyDomainCe_id,
    policyDomainFsimMasterCE_id,
    policyDomainHcPlaced_id,
    policyDomainHcLeafPlace_id,
    policyDomainHcMasterLeafPlace_id,
    policyDomainMax_id
} policyDomainType_t;

extern const char * policyDomain_types [];

#include "policy-domain/hc/hc-policy.h"
#include "policy-domain/ce/ce-policy.h"
#include "policy-domain/xe/xe-policy.h"

ocrPolicyDomainFactory_t * newPolicyDomainFactory(policyDomainType_t type, ocrParamList_t *perType);

#endif /* __POLICY_DOMAIN_ALL_H_ */
