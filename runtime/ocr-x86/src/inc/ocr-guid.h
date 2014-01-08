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

#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef enum {
    OCR_GUID_NONE = 0,
    OCR_GUID_ALLOCATOR = 1,
    OCR_GUID_DB = 2,
    OCR_GUID_EDT = 3,
    OCR_GUID_EDT_TEMPLATE = 4,
    OCR_GUID_EVENT = 5,
    OCR_GUID_POLICY = 6,
    OCR_GUID_WORKER = 7,
    OCR_GUID_MEM_TARGET = 8,
    OCR_GUID_COMP_TARGET = 9
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
struct _ocrPolicyDomain_t;

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
     * @brief "Starts" the GUID provider
     *
     * @param[in] self      Pointer to this GUID provider
     * @param[in] pd        Policy domain this provider belongs to
     */
    void (*start)(struct _ocrGuidProvider_t* self, struct _ocrPolicyDomain_t* pd);
    
    void (*stop)(struct _ocrGuidProvider_t* self);
    void (*finish)(struct _ocrGuidProvider_t* self);

    /**
     * @brief Gets a GUID for an object of kind 'kind'
     * and associates the value val.
     *
     * The GUID provider basically associates a value with the
     * GUID
     *
     * @param[in] self          Pointer to this GUID provider
     * @param[out] guid         GUID returned
     * @param[in] val           Value to be associated
     * @param[in] kind          Kind of the object that will be associated with the GUID
     * @return 0 on success or an error code
     */
    u8 (*getGuid)(struct _ocrGuidProvider_t* self, ocrGuid_t* guid, u64 val,
                  ocrGuidKind kind);

    /**
     * @brief Create a GUID for an object of kind 'kind'
     * and creates storage of size 'size' associated with the
     * GUID
     *
     * getGuid() will associate an existing 64-bit value to a
     * GUID but createGuid() will create storage of size 'size'
     * and associate the resulting address with a GUID. This
     * is useful to create metadata storage
     *
     *
     * @param[in] self          Pointer to this GUID provider
     * @param[out] fguid        GUID returned (with metaDataPtr)
     * @param[in] size          Size of the storage to be created
     * @param[in] kind          Kind of the object that will be associated with the GUID
     * @return 0 on success or an error code
     */
    u8 (*createGuid)(struct _ocrGuidProvider_t* self, ocrFatGuid_t* fguid,
                     u64 size, ocrGuidKind kind);

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
     * @param[in] self        Pointer to this GUID provider
     * @param[in] guid        GUID to release
     * @param[in] releaseVal  If true, will also "free" the value associated
     *                        with the GUID
     * @return 0 on success or an error code
     */
    u8 (*releaseGuid)(struct _ocrGuidProvider_t *self, ocrFatGuid_t guid, bool releaseVal);
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
    struct _ocrPolicyDomain_t *pd;  /**< Policy domain of this GUID provider */
    u32 id;                         /**< Function IDs for this GUID provider */
    ocrGuidProviderFcts_t fcts;     /**< Functions for this instance */
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

    u32 factoryId;
    ocrGuidProviderFcts_t providerFcts; /**< Function pointers created instances should use */
} ocrGuidProviderFactory_t;

/****************************************************/
/* OCR GUID CONVENIENCE FUNCTIONS                   */
/****************************************************/

static inline u8 guidKind(struct _ocrPolicyDomain_t * pd, ocrGuid_t guid,
                          ocrGuidKind* kindRes) __attribute__((unused));

/**
 * @brief Associates a GUID with val
 *
 * This call will cause a GUID to be associated with 'val' but
 * will not create any metadata storage (ie: if val is the pointer
 * to the metadata, it should already be valid)
 *
 * @param[in] pd            Policy domain (current one)
 * @param[in] val           Value to associate
 * @param[out] guidRes      Fat GUID containing the new GUID and val as metaDataPtr
 * @param[in] kind          Kind of the GUID to create
 * @return 0 on success and a non-zero value on failure
 */
static inline u8 guidify(struct _ocrPolicyDomain_t * pd, u64 val, ocrFatGuid_t * guidRes,
                         ocrGuidKind kind) __attribute__((unused));

/**
 * @brief Returns the value associated with GUID which allows the caller
 * to access the meta-data associated with the GUID
 *
 * @param[in] pd            Policy domain (current one)
 * @param[in/out] res       Fat-GUID to 'deguidify'. Note that if
 *                          metaDataPtr is non-NULL, this call is a no-op.
 *                          On input, res.guid should be valid
 * @param[out] kindRes      (optional) If non-NULL, returns kind of GUID
 * @return 0 on success and a non-zero value on failure
 */
static inline u8 deguidify(struct _ocrPolicyDomain_t * pd, ocrFatGuid_t *res,
                           ocrGuidKind* kindRes) __attribute__((unused));

static inline bool isDatablockGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEventGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEdtGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEventLatchGuid(ocrGuid_t guid) __attribute__((unused));
static inline bool isEventSingleGuid(ocrGuid_t guid) __attribute__((unused));

#endif /* __OCR_GUID__H_ */
