/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "policy-domain/policy-domain-all.h"
#include "debug.h"

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

ocrPolicyDomainFactory_t * newPolicyDomainFactory(policyDomainType_t type, ocrParamList_t *perType) {
    switch(type) {
#ifdef ENABLE_POLICY_DOMAIN_HC
    case policyDomainHc_id:
        return newPolicyDomainFactoryHc(perType);
#endif
#ifdef ENABLE_POLICY_DOMAIN_XE
    case policyDomainXe_id:
        return newPolicyDomainFactoryXe(perType);
#endif
#ifdef ENABLE_POLICY_DOMAIN_CE
    case policyDomainCe_id:
        return newPolicyDomainFactoryCe(perType);
#endif
    default:
        ASSERT(0);
    }
    return NULL;
}

void initializePolicyDomainOcr(ocrPolicyDomainFactory_t * factory, ocrPolicyDomain_t * self, ocrParamList_t *perInstance) {
    self->fcts = factory->policyDomainFcts;

    self->myLocation = ((paramListPolicyDomainInst_t*)perInstance)->location;

    self->schedulers = NULL;
    self->allocators = NULL;
    self->commApis = NULL;
    self->shutdownCode = 0;
}
