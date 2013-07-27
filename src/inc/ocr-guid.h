/**
 * @brief OCR internal API to GUID management
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_GUID_H__
#define __OCR_GUID_H__

#include "ocr-types.h"
#include "ocr-mappable.h"
#include "ocr-utils.h"

typedef enum {
    OCR_GUID_NONE = 0,
    OCR_GUID_ALLOCATOR = 1,
    OCR_GUID_DB = 2,
    OCR_GUID_EDT = 3,
    OCR_GUID_EDT_TEMPLATE = 4,
    OCR_GUID_EVENT = 5,
    OCR_GUID_POLICY = 6,
    OCR_GUID_WORKER = 7
} ocrGuidKind;


/****************************************************/
/* OCR PARAMETER LISTS                              */
/****************************************************/

/**
 * @brief Parameter list to create a guid provider factory
 */
typedef struct _paramListGuidProviderFact_t {
    ocrParamList_t base;
} paramListGuidProviderFact_t;

/**
 * @brief Parameter list to create a guid provider instance
 */
typedef struct _paramListGuidProviderInst_t {
    ocrParamList_t base;
} paramListGuidProviderInst_t;


/****************************************************/
/* OCR GUID PROVIDER                                */
/****************************************************/

struct _ocrGuidProvider_t;

/**
 * @brief GUID provider function pointers
 *
 * The function pointers are separate from the GUID provider instance to allow 
 * for the sharing of function pointers for GUID provider from the same factory
 */
typedef struct _ocrGuidProviderFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * This will free the GUID provider and any
     * memory that it uses
     *
     * @param self          Pointer to this GUID provider
     */
    void (*destruct)(struct _ocrGuidProvider_t* self);

    /**
     * @brief Allocates a GUID for an object of kind 'kind'
     * and associates the value val.
     *
     * The GUID provider basically associates a value with the
     * GUID (can be a pointer to the metadata for example).
     *
     * \param[in] self          Pointer to this GUID provider
     * \param[out] guid         GUID returned
     * \param[in] val           Value to be associated
     * \param[in] kind          Kind of the object that will be associated with the GUID
     * @return 0 on success or an error code
     */
    u8 (*getGuid)(struct _ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val,
                  ocrGuidKind kind);

    /**
     * @brief Resolve the associated value to the GUID 'guid'
     *
     * \param[in] self          Pointer to this GUID provider
     * \param[in] guid          GUID to resolve
     * \param[out] val          Parameter-result for the value to return
     * \param[out] kind         Parameter-result for the GUID's kind. Can be NULL if the user does not care about the kind
     *
     * @return 0 on success or an error code
     */
    u8 (*getVal)(struct _ocrGuidProvider_t* self, ocrGuid_t guid, u64* val, ocrGuidKind* kind);

    /**
     * @brief Resolve the kind of a GUID
     *
     * \param[in] self          Pointer to this GUID provider
     * \param[in] guid          GUID to get the kind of
     * \param[out] kind         Parameter-result for the GUID's kind.
     * @return 0 on success or an error code
     */
    u8 (*getKind)(struct _ocrGuidProvider_t* self, ocrGuid_t guid, ocrGuidKind* kind);

    /**
     * @brief Releases the GUID
     *
     * Whether the GUID provider will re-issue this same GUID for a different
     * object is implementation dependent.
     *
     * @param self          Pointer to this GUID provider
     * @param guid          GUID to release
     * @return 0 on success or an error code
     */
    u8 (*releaseGuid)(struct _ocrGuidProvider_t *self, ocrGuid_t guid);
} ocrGuidProviderFcts_t;

/**
 * @brief GUIDs Provider for the system
 *
 * GUIDs should be unique and are used to
 * identify and locate objects (and their associated
 * metadata mostly). GUIDs serve as a level of indirection
 * to allow objects to move around in the system and
 * support different address spaces (in the future)
 */
typedef struct _ocrGuidProvider_t {
    ocrMappable_t module; /**< Base "class" */
    ocrGuidProviderFcts_t *fctPtrs; /**< Function pointers for this instance */
} ocrGuidProvider_t;


/****************************************************/
/* OCR GUID PROVIDER FACTORY                        */
/****************************************************/

/**
 * @brief GUID provider factory
 */
typedef struct _ocrGuidProviderFactory_t {
    /**
     * @brief Instantiate a new GUID provider and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrGuidProvider_t* (*instantiate)(struct _ocrGuidProviderFactory_t *factory, ocrParamList_t* instanceArg);

    /**
     * @brief GUID provider factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrGuidProviderFactory_t *factory);

    ocrGuidProviderFcts_t providerFcts; /**< Function pointers created instances should use */
} ocrGuidProviderFactory_t;

#define UNINITIALIZED_GUID ((ocrGuid_t)-2)

#define ERROR_GUID ((ocrGuid_t)-1)


/****************************************************/
/* OCR GUID CONVENIENCE FUNCTIONS                   */
/****************************************************/

static inline u8 guidKind(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid,
                          ocrGuidKind* kindRes) __attribute__((unused));
// TODO: REC: Actually pass a context
static inline u8 guidify(struct _ocrPolicyDomain_t * pd, u64 ptr, ocrGuid_t * guidRes,
                         ocrGuidKind kind) __attribute__((unused));
static inline u8 deguidify(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid, u64* ptrRes,
                           ocrGuidKind* kindRes) __attribute__((unused));
static inline bool isDatablockGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEventGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEdtGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEventLatchGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEventSingleGuid(ocrGuid_t guid) __attribute__((unused));

#endif /* __OCR_GUID__H_ */
