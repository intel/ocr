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

#include "ocr-policy.h"
#include "ocr-task-event.h"

ocr_policy_domain_t * newPolicy(ocr_policy_kind policyType,
                                size_t nb_workpiles,
                                size_t nb_workers,
                                size_t nb_executors,
                                size_t nb_schedulers) {
    switch(policyType) {
    case OCR_POLICY_HC:
        return hc_policy_domain_constructor(nb_workpiles, nb_workers,
                                            nb_executors, nb_schedulers);
/* sagnak begin */
    case OCR_POLICY_XE:
        return xe_policy_domain_constructor(nb_workpiles, nb_workers,
                                            nb_executors, nb_schedulers);
    case OCR_POLICY_CE:
        return ce_policy_domain_constructor(nb_workpiles, nb_workers,
                                            nb_executors, nb_schedulers);
    case OCR_POLICY_MASTERED_CE:
        return ce_mastered_policy_domain_constructor(nb_workpiles, nb_workers,
                                                     nb_executors, nb_schedulers);
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

    for(i = 0; i < policy->nb_executors; i++) {
        policy->executors[i]->destruct(policy->executors[i]);
    }
    free(policy->executors);

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
