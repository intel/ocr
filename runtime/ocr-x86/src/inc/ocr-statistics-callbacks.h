/**
 * @brief OCR callbacks to use for the various statistics events
 *
 * These functions should be used by the rest of the runtime when
 * a specific event happens that needs to be tracked by the statistics
 * framework. These functions are not meant to be modified and are provided
 * as a convenience and to promote consistency across the implementations
 * of the various modules
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS

#ifndef __OCR_STATISTICS_CALLBACK_H__
#define __OCR_STATISTICS_CALLBACK_H__

#include "ocr-types.h"

typedef struct _ocrAllocator_t ocrAllocator_t;
typedef struct _ocrCompTarget_t ocrCompTarget_t;
typedef struct _ocrDataBlock_t ocrDataBlock_t;
typedef struct _ocrEvent_t ocrEvent_t;
typedef struct _ocrMemTarget_t ocrMemTarget_t;
typedef struct _ocrPolicyDomain_t ocrPolicyDomain_t;
typedef struct _ocrTaskTemplate_t ocrTaskTemplate_t;
typedef struct _ocrTask_t ocrTask_t;
typedef struct _ocrWorker_t ocrWorker_t;

// TODO: Add all documentation

/**
 * @brief Function to use when creating a template
 *
 * As for all the functions in this section, the non-GUID parameter
 * is optional (can be NULL). If provided, it will save on the deguidify
 * operation to extract the metadata structure from the GUID
 */
void statsTEMP_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                      ocrGuid_t templateGuid, ocrTaskTemplate_t *ttemplate);

void statsTEMP_USE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                   ocrGuid_t templateGuid, ocrTaskTemplate_t *ttemplate);

void statsTEMP_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                       ocrGuid_t templateGuid, ocrTaskTemplate_t *ttemplate);

void statsEDT_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                     ocrGuid_t createGuid, ocrTask_t *createTask);

void statsEDT_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                      ocrGuid_t destroyGuid, ocrTask_t *destroyTask);

void statsEDT_START(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                    ocrGuid_t edtGuid, ocrTask_t *task, u8 usesDb);

void statsEDT_END(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                  ocrGuid_t edtGuid, ocrTask_t *task);

void statsDB_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                    ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                    ocrGuid_t dbGuid, ocrDataBlock_t *db);

void statsDB_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                     ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                     ocrGuid_t dbGuid, ocrDataBlock_t *db);

void statsDB_ACQ(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                 ocrGuid_t dbGuid, ocrDataBlock_t *db);

void statsDB_REL(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task, ocrGuid_t dbGuid,
                 ocrDataBlock_t *db);

void statsDB_ACCESS(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                    ocrGuid_t dbGuid, ocrDataBlock_t *db,
                    u64 offset, u64 size, u8 isWrite);

void statsEVT_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                     ocrGuid_t evtGuid, ocrEvent_t *evt);

void statsEVT_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                      ocrGuid_t evtGuid, ocrEvent_t *evt);

void statsDEP_SATISFYToEvt(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                           ocrGuid_t evtGuid, ocrEvent_t *evt, ocrGuid_t dbGuid, u32 slot);

void statsDEP_SATISFYFromEvt(ocrPolicyDomain_t *pd, ocrGuid_t evtGuid, ocrEvent_t *evt,
                             ocrGuid_t destGuid, ocrGuid_t dbGuid, u32 slot);

void statsDEP_ADD(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                  ocrGuid_t depSrcGuid, ocrGuid_t depDestGuid,
                  ocrTask_t *depDest, u32 slot);

void statsWORKER_START(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                       ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget);

void statsWORKER_STOP(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                      ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget);

void statsALLOCATOR_START(ocrPolicyDomain_t *pd, ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                          ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget);

void statsALLOCATOR_STOP(ocrPolicyDomain_t *pd, ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                         ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget);

void statsCOMPTARGET_START(ocrPolicyDomain_t *pd, ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget);

void statsCOMPTARGET_STOP(ocrPolicyDomain_t *pd, ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget);

void statsMEMTARGET_START(ocrPolicyDomain_t *pd, ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget);

void statsMEMTARGET_STOP(ocrPolicyDomain_t *pd, ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget);

#endif /* __OCR_STATISTICS_CALLBACK_H__ */
#endif /* OCR_ENABLE_STATISTICS */
