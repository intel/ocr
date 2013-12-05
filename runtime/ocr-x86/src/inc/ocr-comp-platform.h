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

#include "ocr-types.h"
#include "ocr-utils.h"


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
struct _launchArg_t; // Defined in ocr-comp-target.h
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

    /**
     * @brief Starts a comp-platform (a thread of execution).
     *
     * @param[in] self          Pointer to this comp-platform
     * @param[in] PD            The policy domain bringing up the runtime
     * @param[in] launchArg     Arguments passed to the comp-platform to launch
     */
    void (*start)(struct _ocrCompPlatform_t *self, struct _ocrPolicyDomain_t * PD,
                  struct _launchArg_t * launchArg);

    /**
     * @brief Stops this comp-platform
     * @param[in] self          Pointer to this comp-platform
     */
    void (*stop)(struct _ocrCompPlatform_t *self);

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
     * @brief Send a message to another target
     *
     * This call, which executes on the compute platform
     * making the call, will send an asynchronous message to target. There is
     * no expectation of a result to be sent back but the target may send
     * "back" a one-way message at a later point.
     *
     * The exact implementation of the message sent is implementation dependent
     * and may even be a fully synchronous call but users of this call
     * should assume asynchrony. The exact communication protocol is also
     * implementation dependent.
     *
     * @param[in] self        Pointer to this comp-platform
     * @param[in] target      Comp-platform to communicate with
     * @param[in/out] message Message to send. In the general case, this is
     *                        used as input and instructs the comp-platform
     *                        to send the message given but it may be updated
     *                        and should be used if waitMessage is called
     *                        later.
     * @return 0 on success and a non-zero error code
     */
    u8 (*sendMessage)(struct _ocrCompPlatform_t* self,
                      struct _ocrCompPlatform_t* target,
                      struct _ocrPolicyMsg_t *message);

    /**
     * @brief Checks if a message has been received by the comp platform and,
     * if so, will return a pointer to that message in 'message'
     *
     * The use case for this function is for the worker "loop" to
     * periodically check if it has services to perform for other policy domains
     *
     * @param self[in]        Pointer to this comp-platform
     * @param message[out]    If a message is available, its pointer will be
     *                        returned here
     * @return #POLL_MO_MESSAGE, #POLL_MORE_MESSAGE, #POLL_ERR_MASK
     */
    u8 (*pollMessage)(struct _ocrCompPlatform_t *self, struct _ocrPolicyMsg_t **message);

    /**
     * @brief Waits for a response to the message 'message'
     *
     * This call should only be called in very limited circumstances when the
     * originating call that generated the message is synchronous (for example
     * user calls to OCR).
     *
     * This call will block until message has received a response. Make sure
     * to pass the same message as the one given to sendMessage (as it may
     * have been modified by sendMessage for book-keeping purposes)
     *
     * @return 0 on success and a non-zero error code
     */
    u8 (*waitMessage)(struct _ocrCompPlatform_t *self, struct _ocrPolicyMsg_t *message);

} ocrCompPlatformFcts_t;

/**
 * @brief Interface to a comp-platform representing a
 * resource able to perform computation.
 */
typedef struct _ocrCompPlatform_t {
    ocrCompPlatformFcts_t *fctPtrs; /**< Function pointers for this instance */
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

    /**
     * @brief comp-platform factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCompPlatformFactory_t *factory);

    /**
     * @brief Allows to setup global function pointers
     * @param factory       Pointer to this factory
     * @todo May dissapear
     */
    void (*setIdentifyingFunctions)(struct _ocrCompPlatformFactory_t *factory);

    ocrCompPlatformFcts_t platformFcts; /**< Function pointers created instances should use */
} ocrCompPlatformFactory_t;

#endif /* __OCR_COMP_PLATFORM_H__ */
