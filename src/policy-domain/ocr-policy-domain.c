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

#include <stdlib.h>
#include <assert.h>

#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-task.h"
#include "ocr-event.h"

inline ocrTaskFactory_t* getTaskFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->taskFactory;
}

inline ocrTaskTemplateFactory_t* getTaskTemplateFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->taskTemplateFactory;
}

inline ocrEventFactory_t* getEventFactoryFromPd(ocrPolicyDomain_t *policy) {
 return policy->eventFactory;   
}

inline ocrDataBlockFactory_t* getDataBlockFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->dbFactory;
}

inline ocrLockFactory_t* getLockFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->lockFactory;
}

extern ocrPolicyDomain_t * hc_policy_domain_constructor();
extern ocrPolicyDomain_t * xe_policy_domain_constructor();
extern ocrPolicyDomain_t * ce_policy_domain_constructor();
extern ocrPolicyDomain_t * ce_mastered_policy_domain_constructor();
extern ocrPolicyDomain_t * place_policy_domain_constructor();
extern ocrPolicyDomain_t * leaf_place_policy_domain_constructor();
extern ocrPolicyDomain_t * mastered_leaf_place_policy_domain_constructor();

ocrPolicyDomain_t * newPolicyDomain(ocr_policy_domain_kind policyType,
                                u64 workpileCount,
                                u64 workerCount,
                                u64 computeCount,
                                u64 schedulerCount) {
    switch(policyType) {
    case OCR_POLICY_HC:
        return hc_policy_domain_constructor(workpileCount, workerCount,
                                            computeCount, schedulerCount);
/* sagnak begin */
    case OCR_POLICY_XE:
        return xe_policy_domain_constructor(workpileCount, workerCount,
                                            computeCount, schedulerCount);
    case OCR_POLICY_CE:
        return ce_policy_domain_constructor(workpileCount, workerCount,
                                            computeCount, schedulerCount);
    case OCR_POLICY_MASTERED_CE:
        return ce_mastered_policy_domain_constructor(workpileCount, workerCount,
                                                     computeCount, schedulerCount);
    case OCR_PLACE_POLICY:
        return place_policy_domain_constructor();
    case OCR_LEAF_PLACE_POLICY:
        return leaf_place_policy_domain_constructor(workpileCount, workerCount,
                                                    computeCount, schedulerCount);
    case OCR_MASTERED_LEAF_PLACE_POLICY:
        return mastered_leaf_place_policy_domain_constructor(workpileCount, workerCount,
                                                             computeCount, schedulerCount);
/* sagnak end*/
    default:
        assert(false && "Unrecognized policy domain kind");
        break;
    }
    return NULL;
}

void ocr_policy_domain_destruct(ocrPolicyDomain_t * policy) {
    u64 i;
    for(i = 0; i < policy->workerCount; i++) {
        policy->workers[i]->fctPtrs->destruct(policy->workers[i]);
    }
    free(policy->workers);

    for(i = 0; i < policy->computeCount; i++) {
        policy->computes[i]->fctPtrs->destruct(policy->computes[i]);
    }
    free(policy->computes);

    for(i = 0; i < policy->workpileCount; i++) {
        policy->workpiles[i]->fctPtrs->destruct(policy->workpiles[i]);
    }
    free(policy->workpiles);

    for(i = 0; i < policy->schedulerCount; i++) {
        policy->schedulers[i]->fctPtrs->destruct(policy->schedulers[i]);
    }
    free(policy->schedulers);

    // Destroy the GUID
    ocrGuidProvider_t * guidProvider = getCurrentPD()->guidProvider;
    guidProvider->fctPtrs->releaseGuid(guidProvider, policy->guid);
    free(policy);
}

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
