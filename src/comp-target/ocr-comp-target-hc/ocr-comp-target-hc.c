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

#include <assert.h>
#include <stdlib.h>

#include "ocr-macros.h"
#include "ocr-comp-target.h"
#include "hc.h"

// Fwd declaration
ocrCompTarget_t* newCompTargetHc(ocrCompTargetFactory_t * factory, void * perTypeConfig, void * perInstanceConfig);

void destructCompTargetFactoryHc(ocrCompTargetFactory_t * factory) {
    free(factory);
}

ocrCompTargetFactory_t * newOcrCompTargetFactoryHc(void * config) {
    ocrCompTargetFactoryHc_t* derived = (ocrCompTargetFactoryHc_t*) checkedMalloc(derived, sizeof(ocrCompTargetFactoryHc_t));
    ocrCompTargetFactory_t* base = (ocrCompTargetFactory_t*) derived;
    base->instantiate = newCompTargetHc;
    base->destruct = destructCompTargetFactoryHc;
    return base;
}

typedef struct {
    ocrCompTarget_t base;
} ocrCompTargetHc_t;

void hc_ocr_module_map_worker_to_comp_target(void * self_module, ocrMappableKind kind,
                                           u64 nb_instances, void ** ptr_instances) {
    // Checking mapping conforms to what we're expecting in this implementation
    assert(kind == OCR_WORKER);
    assert(nb_instances == 1);
    ocrWorker_t * worker = (ocrWorker_t *) ptr_instances[0];
    ocrCompTarget_t * compTarget = (ocrCompTarget_t *) self_module;
    //TODO the routine thing is a hack.
    compTarget->platform->routine = worker->routine;
    compTarget->platform->routine_arg = worker;
}

void ocr_comp_target_hc_start(ocrCompTarget_t * compTarget) {
  compTarget->platform->start(compTarget->platform);
}

void ocr_comp_target_hc_stop(ocrCompTarget_t * compTarget) {
  compTarget->platform->stop(compTarget->platform);
}

void ocr_comp_target_hc_destruct (ocrCompTarget_t * compTarget) {
    compTarget->platform->destruct(compTarget->platform);
    free(compTarget);
}

ocrCompTarget_t * newCompTargetHc(ocrCompTargetFactory_t * factory, void * perTypeConfig, void * perInstanceConfig) {
    //TODO the comp-target/comp-platform mapping should be arranged in the policy-domain
    ocrCompPlatform_t * compPlatform = ocr_comp_platform_pthread_constructor();    
    ocrCompTarget_t * compTarget = checkedMalloc(compTarget, sizeof(ocrCompTargetHc_t));
    ocrMappable_t * module_base = (ocrMappable_t *) compTarget;
    module_base->mapFct = hc_ocr_module_map_worker_to_comp_target;
    compTarget->destruct = ocr_comp_target_hc_destruct;
    compTarget->start = ocr_comp_target_hc_start;
    compTarget->stop = ocr_comp_target_hc_stop;
    compTarget->platform = compPlatform;
    compTarget->platform->create(compTarget->platform, perTypeConfig);
    return compTarget;
}
