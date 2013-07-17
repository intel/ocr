/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __POLICY_DOMAIN_ALL_H_
#define __POLICY_DOMAIN_ALL_H_

#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-utils.h"

typedef enum _policyDomainType_t {
    policyDomainHc_id,
    policyDomainFsimXE_id,
    policyDomainFsimCE_id,
    policyDomainFsimMasterCE_id,
    policyDomainHcPlaced_id,
    policyDomainHcLeafPlace_id,
    policyDomainHcMasterLeafPlace_id,
    policyDomainMax_id
} policyDomainType_t;

const char * policyDomain_types [] = {
    "HC",
    "XE",
    "CE",
    "MasterCE",
    "HCPlaced",
    "HCLeafPlace",
    "HCMasterLeafPlace",
    NULL
};

#include "policy-domain/hc/hc-policy.h"

inline ocrPolicyDomainFactory_t * newPolicyDomainFactory(policyDomainType_t type, ocrParamList_t *perType) {
    switch(type) {
    case policyDomainHc_id:
        return newPolicyDomainFactoryHc(perType);
    case policyDomainFsimXE_id:
//        return newPolicyDomainFactoryFsimXE(perType);
    case policyDomainFsimCE_id:
//        return newPolicyDomainFactoryFsimCE(perType);
    case policyDomainFsimMasterCE_id:
//        return newPolicyDomainFactoryFsimMasterCE(perType);
    case policyDomainHcPlaced_id:
//        return newPolicyDomainFactoryHcPlaced(perType);
    case policyDomainHcLeafPlace_id:
//        return newPolicyDomainFactoryHcLeafPlace(perType);
    case policyDomainHcMasterLeafPlace_id:
//        return newPolicyDomainFactoryHcMasterLeafPlace(perType);
    default:
        ASSERT(0);
    }
    return NULL;
}

typedef enum _policyCtxType_t {
    policyCtxHc_id,
    policyCtxMax_id
} policyCtxType_t;

const char * policyCtx_types [] = {
    "HC",
    NULL
};

inline ocrPolicyCtxFactory_t * newPolicyCtxFactory(policyCtxType_t type, ocrParamList_t *perType) {
    switch(type) {
    case policyCtxHc_id:
        return newPolicyCtxFactoryHc(perType);
    default:
        ASSERT(0);
    }
    return NULL;
}

/*
 *
ocrGuid_t policy_domain_handIn_assert ( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    assert(0 && "postponed policy handIn implementation");
    return NULL_GUID;
}

ocrGuid_t policy_domain_extract_assert ( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    assert(0 && "postponed policy extract implementation");
    return NULL_GUID;
}

void policy_domain_handOut_assert ( ocrPolicyDomain_t * this, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid ) {
    assert(0 && "postponed policy handOut implementation");
}

void policy_domain_receive_assert ( ocrPolicyDomain_t * this, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid ) {
    assert(0 && "postponed policy receive implementation");
}
*/

#endif /* __POLICY_DOMAIN_ALL_H_ */
