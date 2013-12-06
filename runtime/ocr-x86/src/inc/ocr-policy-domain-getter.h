/**
 * @brief This file contains how to get to the policy domain
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_POLICY_DOMAIN_GETTER_H_
#define OCR_POLICY_DOMAIN_GETTER_H_

#include "ocr-types.h"

struct _ocrPolicyDomain_t;
struct _ocrPolicyMsg_t;

#if 0
/****************************************************/
/* WRAPPER FUNCTIONS (FOR USER CALLS)               */
/****************************************************/

/**
 * @brief Request the allocation of memory (a data-block)
 *
 * This call will be triggered by user code when a data-block
 * needs to be allocated
 *
 * @param[in/out] guid       Contains the DB GUID to use or, if NULL, will
 *                           contain the resulting GUID
 * @param[out]    ptr        Contains the address for accessing this DB on
 *                           return
 * @param[in]     size       Size of the DB requested
 * @param[in]     properties Properties
 * @param[in]     affinity   Affinity hint on where to allocate
 * @param[in]     allocator  Internal allocator to use
 * @return 0 on success or a non zero value on failure
 */
u8 ocrPDAllocateDb(ocrGuid_t *guid, void** ptr, u64 size, u32 properties,
                   ocrGuid_t affinity, ocrInDbAllocator_t allocator);

/**
 * @brief Request the creation of a task metadata (EDT)
 *
 * This call will be triggered by user code when an EDT
 * is created
 *
 * @todo Improve description to be more like allocateDb
 *
 * @todo Add something about templates here and potentially
 * known dependences so that it can be optimally placed
 */
u8 ocrPDEdtCreate(ocrGuid_t *guid, ocrGuid_t edtTemplate, u32 paramc,
                  u64* paramv, u32 depc, u32 properties, ocrGuid_t affinity,
                  ocrGuid_t * outputEvent);

u8 ocrPDEdtDestroy(ocrGuid_t guid);

/**
 * @brief Request the creation of a task-template metadata
 */
u8 ocrPDCreateEdtTemplate(ocrGuid_t *guid, ocrEdt_t funcPtr, u32 paramc,
                          u32 depc, ocrGuid_t affinity, const char* funcName);

/**
 * @brief Request the creation of an event
 */
u8 ocrPDEventCreate(ocrGuid_t *guid, ocrEventTypes_t type, ocrGuid_t
                    affinity, bool takesArg);

u8 ocrPDEventDestroy(ocrGuid_t );

u8 ocrPDGetGuid(ocrGuid_t *guid, u64 val, ocrGuidKind type);

u8 ocrPDGetInfoForGuid(ocrGuid_t guid, u64* val, ocrGuidKind* type);
                           
#endif

/**
 * @brief Gets the current policy domain for the calling code
 * @todo Will be removed
 */
extern struct _ocrPolicyDomain_t * (*getCurrentPD)();
extern void (*setCurrentPD)(struct _ocrPolicyDomain_t*);
extern struct _ocrPolicyDomain_t * (*getMasterPD)();

extern void (*getCurrentEnv)(struct _ocrPolicyDomain_t **pd, struct _ocrPolicyMsg_t *msg);

#endif /* OCR_POLICY_DOMAIN_GETTER_H_ */
