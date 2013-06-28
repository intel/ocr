/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __POLICY_DOMAIN_ALL_H_
#define __POLICY_DOMAIN_ALL_H_

#include <stdlib.h>
#include <assert.h>

#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-task.h"
#include "ocr-event.h"

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

extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryHc(ocrParamList_t *perType);
extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryFsimXE(ocrParamList_t *perType);
extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryFsimCE(ocrParamList_t *perType);
extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryFsimMasterCE(ocrParamList_t *perType);
extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcPlaced(ocrParamList_t *perType);
extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcLeafPlace(ocrParamList_t *perType);
extern ocrPolicyDomainFactory_t * newPolicyDomainFactoryHcMasterLeafPlace(ocrParamList_t *perType);

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
        return newPolicyDomainFactoryHc(perType);
    }
    assert(0);
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
