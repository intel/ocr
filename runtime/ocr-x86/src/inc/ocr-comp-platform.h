/**
 * @brief OCR interface to computation platforms
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_COMP_PLATFORM_H__
#define __OCR_COMP_PLATFORM_H__

#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

struct _ocrCommPlatform_t;
struct _ocrPolicyDomain_t;
struct _ocrWorker_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create a comp-platform factory
 */
typedef struct _paramListCompPlatformFact_t {
    ocrParamList_t base;    /**< Base class */
} paramListCompPlatformFact_t;

/**
 * @brief Parameter list to create a comp-platform instance
 */
typedef struct _paramListCompPlatformInst_t {
    ocrParamList_t base;    /**< Base class */
} paramListCompPlatformInst_t;


/****************************************************/
/* OCR COMPUTE PLATFORM                             */
/****************************************************/

struct _ocrCompPlatform_t;
struct _ocrPolicyMsg_t;

/**
 * @brief Comp-platform function pointers
 *
 * The function pointers are separate from the comp-platform instance to allow for
 * the sharing of function pointers for comp-platform from the same factory
 */
typedef struct _ocrCompPlatformFcts_t {
    /*! \brief Destroys a comp-platform
     */
    void (*destruct)(struct _ocrCompPlatform_t *self);

    void (*begin)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t *PD,
                  ocrWorkerType_t workerType);

    /**
     * @brief Starts a comp-platform (a thread of execution).
     *
     * @param[in] self          Pointer to this comp-platform
     * @param[in] pd            Policy domain on this comp-platform
     * @param[in] worker        Worker runnning on this comp-platform
     */
    void (*start)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t *PD,
                  struct _ocrWorker_t * worker);

    /**
     * @brief Stops this comp-platform
     * @param[in] self          Pointer to this comp-platform
     */
    void (*stop)(struct _ocrCompPlatform_t *self);

    void (*finish)(struct _ocrCompPlatform_t *self);

    /**
     * @brief Gets the throttle value for this compute node
     *
     * A value of 100 indicates nominal throttling.
     *
     * @param[in] self        Pointer to this comp-platform
     * @param[out] value      Throttling value
     * @return 0 on success or the following error code:
     *     - 1 if the functionality is not supported
     *     - other codes implementation dependent
     */
    u8 (*getThrottle)(struct _ocrCompPlatform_t* self, u64 *value);

    /**
     * @brief Sets the throttle value for this compute node
     *
     * A value of 100 indicates nominal throttling.
     *
     * @param[in] self        Pointer to this comp-platform
     * @param[in] value       Throttling value
     * @return 0 on success or the following error code:
     *     - 1 if the functionality is not supported
     *     - other codes implementation dependent
     */
    u8 (*setThrottle)(struct _ocrCompPlatform_t* self, u64 value);

    /**
     * @brief Function called from the worker when it starts "running" on the comp-platform
     *
     * @note This function is separate from the start function because, conceptually,
     * multiple workers could share a comp-platform. The pd argument is used
     * to verify that the worker's PD and the comp-platform's PD match
     *
     * @param[in] self        Pointer to this comp-platform
     * @param[in] pd          Policy domain running on this comp-platform
     * @param[in] worker      Worker running on this comp-platform
     * @return 0 on success and a non-zero error code
     */
    u8 (*setCurrentEnv)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t *pd,
                        struct _ocrWorker_t *worker);

} ocrCompPlatformFcts_t;

/**
 * @brief Interface to a comp-platform representing a
 * resource able to perform computation.
 */
typedef struct _ocrCompPlatform_t {
    struct _ocrPolicyDomain_t *pd;  /**< Policy domain this comp-platform is used by */
    struct _ocrWorker_t * worker;    /**< Worker for this comp platform */
    ocrCompPlatformFcts_t fcts; /**< Functions for this instance */
} ocrCompPlatform_t;


/****************************************************/
/* OCR COMPUTE PLATFORM FACTORY                     */
/****************************************************/

/**
 * @brief comp-platform factory
 */
typedef struct _ocrCompPlatformFactory_t {
    /**
     * @brief Instantiate a new comp-platform and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrCompPlatform_t* (*instantiate)(struct _ocrCompPlatformFactory_t *factory,
                                      ocrParamList_t *instanceArg);

    void (*initialize) (struct _ocrCompPlatformFactory_t * factory, ocrCompPlatform_t *self, ocrParamList_t * perInstance);
    /**
     * @brief comp-platform factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCompPlatformFactory_t *factory);


    /**
     * @brief Sets-up the getCurrentEnv function
     * @param factory       Pointer to this factory
     */
    void (*setEnvFuncs)(struct _ocrCompPlatformFactory_t *factory);

    ocrCompPlatformFcts_t platformFcts; /**< Function pointers created instances should use */
} ocrCompPlatformFactory_t;

void initializeCompPlatformOcr(ocrCompPlatformFactory_t * factory, ocrCompPlatform_t * self, ocrParamList_t *perInstance);

#endif /* __OCR_COMP_PLATFORM_H__ */
