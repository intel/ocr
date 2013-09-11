/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_RUNTIME_MODEL_H_
#define OCR_RUNTIME_MODEL_H_

#include "ocr-policy-domain.h"

/**
 * This is our base model for components we want instantiate
 */
typedef _ocr_model_t struct {
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
    ocrModuleMapping_t * mappings;
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
ocrModuleMapping_t build_ocr_module_mapping(ocrMappingKind kind, ocrMappableKind from, ocrMappableKind to);

#endif /* OCR_RUNTIME_MODEL_H_ */
