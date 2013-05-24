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

#include "ocr-policy-domain.h"
#include "ocr-task-event.h"

ocr_policy_domain_t * newPolicyDomain(ocr_policy_domain_kind policyType,
                                size_t nb_workpiles,
                                size_t nb_workers,
                                size_t nb_comp_targets,
                                size_t nb_schedulers) {
    switch(policyType) {
    case OCR_POLICY_HC:
        return hc_policy_domain_constructor(nb_workpiles, nb_workers,
                                            nb_comp_targets, nb_schedulers);
/* sagnak begin */
    case OCR_POLICY_XE:
        return xe_policy_domain_constructor(nb_workpiles, nb_workers,
                                            nb_comp_targets, nb_schedulers);
    case OCR_POLICY_CE:
        return ce_policy_domain_constructor(nb_workpiles, nb_workers,
                                            nb_comp_targets, nb_schedulers);
    case OCR_POLICY_MASTERED_CE:
        return ce_mastered_policy_domain_constructor(nb_workpiles, nb_workers,
                                                     nb_comp_targets, nb_schedulers);
    case OCR_PLACE_POLICY:
        return place_policy_domain_constructor();
    case OCR_LEAF_PLACE_POLICY:
        return leaf_place_policy_domain_constructor(nb_workpiles, nb_workers,
                                                    nb_comp_targets, nb_schedulers);
    case OCR_MASTERED_LEAF_PLACE_POLICY:
        return mastered_leaf_place_policy_domain_constructor(nb_workpiles, nb_workers,
                                                             nb_comp_targets, nb_schedulers);
/* sagnak end*/
    default:
        assert(false && "Unrecognized policy domain kind");
        break;
    }
    return NULL;
}

void ocr_policy_domain_destruct(ocr_policy_domain_t * policy) {
    size_t i;
    for(i = 0; i < policy->nb_workers; i++) {
        policy->workers[i]->destruct(policy->workers[i]);
    }
    free(policy->workers);

    for(i = 0; i < policy->nb_comp_targets; i++) {
        policy->compTargets[i]->destruct(policy->compTargets[i]);
    }
    free(policy->compTargets);

    for(i = 0; i < policy->nb_workpiles; i++) {
        policy->workpiles[i]->destruct(policy->workpiles[i]);
    }
    free(policy->workpiles);

    for(i = 0; i < policy->nb_schedulers; i++) {
        policy->schedulers[i]->destruct(policy->schedulers[i]);
    }
    free(policy->schedulers);

    // Destroy the GUID
    globalGuidProvider->releaseGuid(globalGuidProvider, policy->guid);
    free(policy);
}

ocr_policy_domain_t* get_current_policy_domain () {
    ocrGuid_t worker_guid = ocr_get_current_worker_guid();
    ocr_worker_t * worker = NULL;
    globalGuidProvider->getVal(globalGuidProvider, worker_guid, (u64*)&worker, NULL);

    ocr_scheduler_t * scheduler = get_worker_scheduler(worker);
    ocr_policy_domain_t* policy_domain = scheduler -> domain;

    return policy_domain;
}

ocrGuid_t policy_domain_handIn_assert ( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    assert(0 && "postponed policy handIn implementation");
    return NULL_GUID;
}

ocrGuid_t policy_domain_extract_assert ( ocr_policy_domain_t * this, ocr_policy_domain_t * takingPolicy, ocrGuid_t takingWorkerGuid ) {
    assert(0 && "postponed policy extract implementation");
    return NULL_GUID;
}

void policy_domain_handOut_assert ( ocr_policy_domain_t * this, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid ) {
    assert(0 && "postponed policy handOut implementation");
}

void policy_domain_receive_assert ( ocr_policy_domain_t * this, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid ) {
    assert(0 && "postponed policy receive implementation");
}
