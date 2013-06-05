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

#ifndef OCR_RUNTIME_MODEL_H_
#define OCR_RUNTIME_MODEL_H_

#include "ocr-policy-domain.h"

/**
 * This is our base model for components we want instantiate
 */
typedef struct {
    int kind;
    u64 nb_instances;
    void * perTypeConfig;
    void ** perInstanceConfig;
} ocr_model_t;

typedef struct _ocrAllocatorModel_t {
    ocr_model_t model;
    u64 sizeManaged;
} ocrAllocatorModel_t;

typedef struct {
    ocr_model_t model;
    u64 nb_scheduler_types;
    u64 nb_worker_types;
    u64 nb_comp_target_types;
    u64 nb_workpile_types;
    u64 nb_mappings;
    u64 numAllocTypes;
    u64 numMemTypes;
    ocr_model_t * schedulers;
    ocr_model_t * workers;
    ocr_model_t * computes;
    ocr_model_t * workpiles;
    ocrAllocatorModel_t * allocators;
    ocr_model_t * memories;
    ocr_module_mapping_t * mappings;
} ocr_model_policy_t;

typedef enum ocr_policy_model_kind_enum {
  OCR_POLICY_MODEL_FLAT = 1,
  OCR_POLICY_MODEL_FSIM = 2,
  OCR_POLICY_MODEL_THOR = 3,
} ocr_policy_model_kind;

ocr_model_policy_t * ocrInitPolicyModel(ocr_policy_model_kind policyModelKind, char * mdFile);

ocrPolicyDomain_t ** instantiateModel(ocr_model_policy_t * model);


void destructOcrModelPolicy(ocr_model_policy_t *);

// THIS IS A HACK RIGHT NOW TO GET THE MEMORY SIZE IN ONE LARGE CHUNK
extern u64 gHackTotalMemSize;

// Hooks for implementations to set what they have instantiated
extern u64 n_root_policy_nodes;
extern ocrPolicyDomain_t ** root_policies;
extern ocrWorker_t* master_worker;

// Build a model
ocr_model_t* newModel ( int kind, int nInstances, void * perTypeConfig, void ** perInstanceConfig );

// Helper function to build ocr module mapping
ocr_module_mapping_t build_ocr_module_mapping(ocr_mapping_kind kind, ocrMappableKind from, ocrMappableKind to);

#endif /* OCR_RUNTIME_MODEL_H_ */
