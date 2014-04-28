/**
 * @brief OCR high-level interface to communication
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_COMM_API_H__
#define __OCR_COMM_API_H__

#include "ocr-runtime-types.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

struct _ocrPolicyDomain_t;
struct _ocrWorker_t;
struct _ocrCommPlatform_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

/**
 * @brief Parameter list to create a comm-api factory
 */
typedef struct _paramListCommApiFact_t {
    ocrParamList_t base;    /**< Base class */
} paramListCommApiFact_t;

/**
 * @brief Parameter list to create a comm-api instance
 */
typedef struct _paramListCommApiInst_t {
    ocrParamList_t base;    /**< Base class */
} paramListCommApiInst_t;


/****************************************************/
/* OCR COMMUNICATION API                            */
/****************************************************/

struct _ocrCommApi_t;
struct _ocrPolicyMsg_t;
struct _ocrMsgHandle_t;

/**
 * @brief Comm-api function pointers
 *
 */
typedef struct _ocrCommApiFcts_t {

    /*! \brief Destroys a comm-api
     */
    void (*destruct)(struct _ocrCommApi_t *self);

    void (*begin)(struct _ocrCommApi_t *self, struct _ocrPolicyDomain_t *PD);

    /**
     * @brief Starts a comm-api (a communication entity).
     *
     * @param[in] self          Pointer to this comm-api
     * @param[in] PD            The policy domain bringing up the runtime
     */
    void (*start)(struct _ocrCommApi_t *self, struct _ocrPolicyDomain_t * PD);

    /**
     * @brief Stops this comm-api
     * @param[in] self          Pointer to this comm-api
     */
    void (*stop)(struct _ocrCommApi_t *self);

    void (*finish)(struct _ocrCommApi_t *self);

    /**
     * @brief Send a message to another target
     *
     * This call will send an asynchronous message to target. The call will,
     * if the user wants it, return a handle to allow polling and/or waiting
     * on a response to this message (if a return value is required).
     *
     * The actual implementation of this function may be a fully synchronous
     * call but users of this call should assume asynchrony.
     *
     * The 'properties' field allows the caller to specify the way the call
     * is being made with regards to the lifetime of 'message' and 'handle':
     *    - if (properties & TWOWAY_MSG_PROP) is non zero:
     *        - An "answer" to this message is expected. In other words, it is
     *          expected that the a poll/wait call at a later point will return
     *          a message that is a "response" to this message.
     *    - if (properties & PERSIST_MSG_PROP) is non zero:
     *        - The caller guarantees that the value pointed to by
     *          message will be valid until either:
     *            - If TWOWAY_MSG_PROP: a poll/wait returns the response and
     *              it is successfully processed. In this case handle->msg
     *              will always point to the input message
     *            - If not TWOWAY_MSG_PROP: the callee will free msg whenever
     *              it wishes using pdFree. In other words, the caller "hands-off"
     *              the freeing of the message buffer to the lower levels and
     *              guarantees that pdFree can be used to free the message
     * If properties is 0:
     *     - the lifetime of 'message' is not guaranteed once the call returns
     *     - no response for this message is expected.
     * Properties can also encode a priority which may or may not be
     * respected.
     *
     * @param[in] self        Pointer to this
     * @param[in] target      Target to communicate with
     * @param[in] message     Message to send
     * @param[out] handle     If non NULL, the callee will return
     *                        a handle that will allow the caller to wait/poll
     *                        for a response to this message. The handle
     *                        includes a 'destruct' function that *MUST* be
     *                        called once the caller is done with the handle.
     *                        Note that a non NULL handle implies
     *                        TWOWAY_MSG_PROP (an error will be returned
     *                        if properties does not contain this)
     * Example usage cases:
     *     - Stack allocated persistent message with response
     * ocrPolicyMsg_t msg;
     * ocrMsgHandle_t *handle;
     * // Fill in msg
     * sendMessage(me, target, &msg, &handle, PERSIST_MSG_PROP | TWOWAY_MSG_PROP);
     * waitMessage(me, handle);
     * // Here:
     * //   - handle->status is RESPONSE_OK (if all went well)
     * //   - handle->msg is equal to &msg
     * // Process response using handle->response
     * handle->destruct(handle); // Destroy handle. This will
     *                           // destroy the handle and handle->response if needed
     *
     *     - Stack allocated non-persistent message with responses
     * // Send 10 messages
     * for(u32 i=0; i<10; ++i) {
     *   ocrPolicyMsg_t msg;
     *   // Fill in msg
     *   sendMessage(me, target, &msg, NULL, TWOWAY_MSG_PROP);
     * }
     * // Go do something else
     * // Now wait for responses to all 10 messages
     * u32 countResp = 0;
     * while(countResp != 10) {
     *   ocrMsgHandle_t *handle = NULL;
     *   pollMessage(me, &handle, 0);
     *   if(handle && handle->status == RESPONSE_OK) {
     *     // Here:
     *     //   - handle->msg is NULL
     *     // Use handle->response
     *     // to process response
     *     handle->destruct(handle);
     *     ++countResp;
     *   }
     * }
     *
     *     - Heap allocated persistent messages with responses
     * // Send 10 messages
     * for(u32 i=0; i<10; ++i) {
     *   ocrPolicyMsg_t *msg = pd->pdMalloc(pd, sizeof(ocrPolicyMsg_t));
     *   // Fill in msg
     *   sendMessage(me, target, msg, NULL, PERSIST_MSG_PROP | TWOWAY_MSG_PROP);
     * }
     * // Go do something else
     * // Now wait for responses to all 10 messages
     * u32 countResp = 0;
     * while(countResp != 10) {
     *   ocrMsgHandle_t *handle = NULL;
     *   pollMessage(me, &handle, 0);
     *   if(handle && handle->status == RESPONSE_OK) {
     *     // Here:
     *     //   - handle->msg is non-NULL
     *     //     and needs to be freed BUT only
     *     //     do it after processing the response as
     *     //     handle->response may be handle->msg
     *
     *     // Use handle->response
     *     // to process message
     *     pd->pdFree(pd, handle->msg);
     *     handle->destruct(handle);
     *     ++countResp;
     *   }
     * }
     *
     * @return 0 on success and a non-zero error code
     */
    u8 (*sendMessage)(struct _ocrCommApi_t* self, ocrLocation_t target,
                      struct _ocrPolicyMsg_t *message,
                      struct _ocrMsgHandle_t **handle, u32 properties);

    /**
     * @brief Non-blocking check for incomming messages
     *
     * This function allows the caller to check for the status of
     * a specific message or ANY message.
     *
     * @param[in] self        Pointer to this
     * @param[in/out] handle  Must never be NULL. If '*handle' is non-NULL,
     *                        this call will poll for a response to that
     *                        specific handle. On return,
     *                        check to see if (*handle)->status is RESPONSE_OK
     *                        The call will also
     *                        return POLL_NO_MESSAGE if no response was found.
     *                        If '*handle' is NULL, the call will return the
     *                        handle of ANY message.
     *                        If no message match, '*handle' will still be
     *                        NULL and POLL_NO_MESSAGE will be returned.
     *
     *
     * After a successful poll, use (*handle)->response as
     * the response message and call (*handle)->destruct(*handle) when done.
     * If (*handle)->msg is non-NULL, this means that the message was
     * persistent when sent (PERSIST_MSG_PROP) and needs to be dealt with by
     * the caller.
     * @return #POLL_NO_MESSAGE, #POLL_MORE_MESSAGE
     *
     * @note If polling with a specific handle, (*handle)->status may be used
     * to determine if the message has been sent but is implementation
     * dependent. See ocrHandleStatus_t
     */
    u8 (*pollMessage)(struct _ocrCommApi_t *self, struct _ocrMsgHandle_t **handle);

    /**
     * @brief Blocking check for incomming messages
     *
     * This call blocks until a response has been received to a particular
     * handle or ANY message has been received.
     *
     * @param[in] self        Pointer to this
     * @param[in/out] handle  The handle of the communication to wait for.
     *                        If (*handle) is non NULL, wait for that
     *                        particular message. Otherwise wait for
     *                        any message. See pollMessage for more detail
     *
     * One return from this call, use handle->response as the
     * response message and call handle->destruct(handle) when done.
     * If handle->msg is non-NULL, this means that the message was
     * persistent when sent (PERSIST_MSG_PROP) and needs to be dealt
     * with by the caller.
     * @note Unlike poll, this call cannot be used to determine if a message
     * has been sent. Apart from that difference, it is a blocking version
     * of pollMessage.
     * @return 0 on success and a non-zero error code
     */
    u8 (*waitMessage)(struct _ocrCommApi_t *self, struct _ocrMsgHandle_t ** handle);
} ocrCommApiFcts_t;


/**
 * @brief Interface to a comm-api representing the high-level
 * communication handling
 */
typedef struct _ocrCommApi_t {
    struct _ocrPolicyDomain_t *pd;  /**< Policy domain this comm-platform is used by */
    struct _ocrCommPlatform_t *commPlatform; /**< Communication platform to use */
    ocrCommApiFcts_t fcts; /**< Functions for this instance */
} ocrCommApi_t;


/****************************************************/
/* OCR COMMUNICATION API FACTORY                    */
/****************************************************/

/**
 * @brief comm-api factory
 */
typedef struct _ocrCommApiFactory_t {
    /**
     * @brief Instantiate a new comm-api and returns a pointer to it.
     *
     * @param factory       Pointer to this factory
     * @param instanceArg   Arguments specific for this instance
     */
    ocrCommApi_t* (*instantiate)(struct _ocrCommApiFactory_t *factory,
                                 ocrParamList_t *instanceArg);

    /**
     * @brief Initialize a comm-api
     *
     * @param factory       Pointer to this factory
     * @param self          The comm-api instance to initialize
     * @param instanceArg   Arguments specific for this instance
     */
    void (*initialize)(struct _ocrCommApiFactory_t * factory, struct _ocrCommApi_t * self, ocrParamList_t * perInstance);

    /**
     * @brief comm-api factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCommApiFactory_t *factory);

    ocrCommApiFcts_t apiFcts; /**< Function pointers created instances should use */
} ocrCommApiFactory_t;

void initializeCommApiOcr(struct _ocrCommApiFactory_t * factory, struct _ocrCommApi_t * self, ocrParamList_t *perInstance);

#endif /* __OCR_COMM_API_H__ */
