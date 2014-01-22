/**
 * @brief OCR interface to communication platforms
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_COMM_PLATFORM_H__
#define __OCR_COMM_PLATFORM_H__

#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

struct _ocrPolicyDomain_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create a comm-platform factory
 */
typedef struct _paramListCommPlatformFact_t {
    ocrParamList_t base;    /**< Base class */
} paramListCommPlatformFact_t;

/**
 * @brief Parameter list to create a comm-platform instance
 */
typedef struct _paramListCommPlatformInst_t {
    ocrParamList_t base;    /**< Base class */
} paramListCommPlatformInst_t;


/****************************************************/
/* OCR COMMUNICATION PLATFORM                       */
/****************************************************/

struct _ocrCommPlatform_t;
struct _ocrPolicyMsg_t;
struct _launchArg_t; // Defined in ocr-comp-target.h
/**
 * @brief Comm-platform function pointers
 *
 * The function pointers are separate from the comm-platform instance to allow for
 * the sharing of function pointers for comm-platform from the same factory
 */
typedef struct _ocrCommPlatformFcts_t {
    /*! \brief Destroys a comm-platform
     */
    void (*destruct)(struct _ocrCommPlatform_t *self);

    void (*begin)(struct _ocrCommPlatform_t *self, struct _ocrPolicyDomain_t *PD,
                  ocrWorkerType_t workerType);

    /**
     * @brief Starts a comm-platform (a communication entity).
     *
     * @param[in] self          Pointer to this comm-platform
     * @param[in] PD            The policy domain bringing up the runtime
     * @param[in] workerType    Type of the worker runnning on this comp-platform
     * @param[in] launchArg     Arguments passed to the comm-platform to launch
     */
    void (*start)(struct _ocrCommPlatform_t *self, struct _ocrPolicyDomain_t * PD,
                  ocrWorkerType_t workerType, struct _launchArg_t * launchArg);

    /**
     * @brief Stops this comp-platform
     * @param[in] self          Pointer to this comp-platform
     */
    void (*stop)(struct _ocrCommPlatform_t *self);

    void (*finish)(struct _ocrCommPlatform_t *self);
    
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
     * @param[in] self        Pointer to this
     * @param[in] target      Target to communicate with
     * @param[in/out] message Message to send. In the general case, this is
     *                        used as input and instructs the comp-target
     *                        to send the message given but it may be updated
     *                        and should be used if waitMessage is called
     *                        later. Note the pointer-to-pointer argument.
     *                        If a new message is created (and its pointer
     *                        returned in the call), the old message will *not*
     *                        be freed and it is up to the caller to properly
     *                        dispose of it (may be on the stack).
     *                        The new message, once fully used, needs to
     *                        be disposed of with pdFree
     * @return 0 on success and a non-zero error code
     */
    u8 (*sendMessage)(struct _ocrCommPlatform_t* self,
                      ocrLocation_t target,
                      struct _ocrPolicyMsg_t **message);

    /**
     * @brief Checks if a message has been received by the comm platform and,
     * if so, will return a pointer to that message in 'message'
     *
     * The use case for this function is for the worker "loop" to
     * periodically check if it has services to perform for other policy domains.
     *
     * If waiting for a specific message, make sure to pass the message returned
     * by sendMessage.
     *
     * @param self[in]        Pointer to this comp-platform
     * @param message[in/out] If a message is available, its pointer will be.
     *                        If *message is NULL on input, this function will
     *                        poll for ANY messages that meet the mask property (see
     *                        below). If *message is non NULL, this function
     *                        will poll for only that specific message.
     *                        The message returned needs to be freed using pdFree.
     * @param mask[in]        If non-zero, this function will only return messages
     *                        whose 'property' variable ORed with this mask returns
     *                        a non-zero value
     * @return #POLL_MO_MESSAGE, #POLL_MORE_MESSAGE, #POLL_ERR_MASK
     */
    u8 (*pollMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t **message, u32 mask);

    /**
     * @brief Waits for a response to the message 'message'
     *
     * This call should only be called in very limited circumstances when the
     * originating call that generated the message is synchronous (for example
     * user calls to OCR).
     *
     * This call will block until message has received a response. Make sure
     * to pass the same message as the one returned by sendMessage (as it may
     * have been modified by sendMessage for book-keeping purposes)
     *
     * @param self[in]        Pointer to this comm-platform
     * @param message[in/out] As input, this determines which message to wait
     *                        for. Note that this is a pointer to a pointer.
     *                        The returned message, once used, needs to be
     *                        freed with pdFree.
     * @return 0 on success and a non-zero error code
     */
    u8 (*waitMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t **message);    
} ocrCommPlatformFcts_t;

/**
 * @brief Interface to a comp-platform representing a
 * resource able to perform computation.
 */
typedef struct _ocrCommPlatform_t {
    struct _ocrPolicyDomain_t *pd;  /**< Policy domain this comm-platform is used by */
    ocrLocation_t location;
    ocrCommPlatformFcts_t fcts; /**< Functions for this instance */
} ocrCommPlatform_t;


/****************************************************/
/* OCR COMPUTE PLATFORM FACTORY                     */
/****************************************************/

/**
 * @brief comm-platform factory
 */
typedef struct _ocrCommPlatformFactory_t {
    /**
     * @brief Instantiate a new comm-platform and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrCommPlatform_t* (*instantiate)(struct _ocrCommPlatformFactory_t *factory,
                                      ocrLocation_t location, ocrParamList_t *instanceArg);

    /**
     * @brief comm-platform factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCommPlatformFactory_t *factory);
    
    ocrCommPlatformFcts_t platformFcts; /**< Function pointers created instances should use */
} ocrCommPlatformFactory_t;

#endif /* __OCR_COMM_PLATFORM_H__ */

