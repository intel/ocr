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

#include "ocr-policy.h"

/**
 * This is our base model for components we want instantiate
 */
typedef struct {
    int kind;
    size_t nb_instances;
    void * configuration;
} ocr_model_t;

typedef struct {
    ocr_model_t model;
    size_t nb_scheduler_types;
    size_t nb_worker_types;
    size_t nb_executor_types;
    size_t nb_workpile_types;
    size_t nb_mappings;
    u64 numAllocTypes;
    u64 numMemTypes;
    ocr_model_t * schedulers;
    ocr_model_t * workers;
    ocr_model_t * executors;
    ocr_model_t * workpiles;
    ocr_model_t * allocators;
    ocr_model_t * memories;
    ocr_module_mapping_t * mappings;
} ocr_model_policy_t;

ocr_model_policy_t * defaultOcrModelPolicy(size_t nb_schedulers, size_t nb_workers,
                                           size_t nb_executors, size_t nb_workpiles);


ocr_policy_domain_t * instantiateModel(ocr_model_policy_t * model);

#endif /* OCR_RUNTIME_MODEL_H_ */
