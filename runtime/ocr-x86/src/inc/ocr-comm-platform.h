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

struct _ocrCommApi_t;
struct _ocrPolicyDomain_t;
struct _ocrPolicyMsg_t;

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
    ocrLocation_t location;
} paramListCommPlatformInst_t;


/****************************************************/
/* OCR COMMUNICATION PLATFORM                       */
/****************************************************/

struct _ocrCommPlatform_t;

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
                  struct _ocrCommApi_t *comm);

    /**
     * @brief Starts a comm-platform (a communication entity).
     *
     * @param[in] self          Pointer to this comm-platform
     * @param[in] PD            The policy domain bringing up the runtime
     * @param[in] workerType    Type of the worker runnning on this comp-platform
     */
    void (*start)(struct _ocrCommPlatform_t *self, struct _ocrPolicyDomain_t * PD,
                  struct _ocrCommApi_t *comm);

    /**
     * @brief Stops this comp-platform
     * @param[in] self          Pointer to this comp-platform
     */
    void (*stop)(struct _ocrCommPlatform_t *self);

    void (*finish)(struct _ocrCommPlatform_t *self);

    /**
     * @brief Tells the communication platfrom that the biggest
     * messages expected are of size 'size'
     *
     * Certain implementations may need/want to pre-allocated
     * structures to receive incomming messages. This indicates how
     * big the biggest message that is expected is for a particular
     * mask (mask of 0 means all messages).
     *
     * This call can be called multiple times if new information about
     * message sizes comes in (for example a query will require a larger
     * response than usual).
     *
     * @param[in] self          Pointer to this
     * @param[in] size          Maximum expected size for the mask given
     * @param[in] mask          Mask (0 means no mask so all messages)
     * @return 0 on success and a non-zero error code
     */
    u8 (*setMaxExpectedMessageSize)(struct _ocrCommPlatform_t *self, u64 size, u32 mask);

    /**
     * @brief Send a message to another target
     *
     * This call, which executes on the compute platform making the call,
     * will send an asynchronous message to target. The call will, if the user
     * wants it, return an ID to check on the status of the message
     *
     * The actual implementation of this function may be a fully synchronous
     * call but users of this call should assume asynchrony.
     *
     * The 'properties' field allows the caller to specify the way the call
     * is being made with regards to the lifetime of 'message':
     *    - if (properties & TWOWAY_MSG_PROP) is non zero:
     *        - An "answer" to this message is expected. This flag is used in
     *          conjunction with PERSIST_MSG_PROP
     *    - if (properties & PERSIST_MSG_PROP) is non zero:
     *        - If TWOWAY_MSG_PROP: the comm-platform can use the message space
     *          to send the message and does not need to free it
     *        - If not TWOWAY_MSG_PROP: the comm-platform must free message using
     *          pdFree when it no longer needs it.
     * To recap the values of properties:
     *     - 0: The comm-platform needs to make a copy of the message or "fully"
     *          send it before returning
     *     - TWOWAY_MSG_PROP: Invalid value (ignored)
     *     - TWOWAY_MSG_PROP | PERSIST_MSG_PROP: The comm-platform can use
     *       msg to send the message (no copy needed). It should NOT free msg
     *     - PERSIST_MSG_PROP: The comm-platform needs to free msg at some point
     *       using pdFree when it has finished using the buffer.
     * The properties can also encode a priority which, if supported, will cause
     * the receiving end's poll/wait to prioritize messages with higher priority
     * if multiple messages are available.
     *
     * The 'bufferSize' parameter indicates to the underlying implementation:
     *   - how big the message to send effectively is
     *   - how big the actual allocation of message really is
     *   - usually, the message to send is much smaller than sizeof(ocrPolicyMsg_t)
     *     due to the use of a union in that structure.
     *
     * The mask parameter allows the underlying implementation to differentiate
     * messages if it wants to (see setMaxExpectedMessageSize())
     *
     * @param[in] self        Pointer to this
     * @param[in] target      Target to communicate with
     * @param[in] message     Message to send
     * @param[in] bufferSize  Bottom 32 bits: actual size of the message to send
     *                        (starting at 'message').
     *                        Top 32 bits: real size of the buffer starting at
     *                        buffer.
     * @param[out] id         If non-NULL, an ID is returned which can be used
     *                        in getMessageStatus. Note that getMessageStatus
     *                        may always return MSG_NORMAL.
     * @param[in] properties  Properties for this send. See above.
     * @param[in] mask        Mask of the outgoing message (may be ignored)
     * @return 0 on success and a non-zero error code
     */
    u8 (*sendMessage)(struct _ocrCommPlatform_t* self, ocrLocation_t target,
                      struct _ocrPolicyMsg_t *message, u64 bufferSize, u64 *id,
                      u32 properties, u32 mask);

    /**
     * @brief Non-blocking check for incomming messages
     *
     * This function checks for ANY incomming message
     * @param[in] self        Pointer to this comm-platform
     * @param[out] msg        Returns a pointer to an ocrPolicyMsg_t. Once
     *                        used by the upper levels, the returned pointer
     *                        needs to be "freed" using destructMessage.
     *                        If *msg is NULL on return, check return code
     *                        to see if there was an error or if no messages
     *                        were available
     * @param[in] properties  Unused for now
     * @param[out] mask       Mask of the returned message (may always be 0)
     * @return
     * #POLL_NO_MESSAGE: 0 messages found
     * 0 : Only 1 message found
     * #POLL_MORE_MESSAGE: 1 message found but more available
     * #POLL_ERR_MASK
     */
    u8 (*pollMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t **msg,
                      u32 properties, u32 *mask);

    /**
     * @brief Blocking check for incomming messages
     *
     * This function checks for ANY incomming message
     * @param[in] self        Pointer to this comm-platform
     * @param[out] msg        Returns a pointer to an ocrPolicyMsg_t. Once
     *                        used by the upper levels, the returned pointer
     *                        needs to be "freed" using destructMessage.
     *                        If *msg is NULL on return, an error has occured
     * @param[in] properties  Unused for now
     * @param[out] mask       Mask of the returned message (may always be 0)
     * @return 0 on success and a non-zero error code
     */
    u8 (*waitMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t **msg,
                      u32 properties, u32 *mask);

    /**
     * @brief Releases/frees a message returned by pollMessage/waitMessage
     *
     * Implementations may vary in how this is implemented. This call must
     * be called at some point after pollMessage/waitMessage on the returned message
     * buffer
     *
     * @param[in] self          Pointer to this comm-platform
     * @param[out] msg          Pointer to the message to destroy. The caller guarantees
     *                          that it is done using that message buffer
     * @return 0 on success and a non-zero error code
     */
    u8 (*destructMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t *msg);

    /**
     * @brief Returns the status of a message sent via sentMessage
     *
     * Note that some implementations may not return any useful status and always
     * return UNKNOWN_MSG_STATUS.
     *
     * @param[in] self          Pointer to this comm-platform
     * @param[in] id            ID returned by sendMessage
     * @return The status of the message
     */
    ocrMsgStatus_t (*getMessageStatus)(struct _ocrCommPlatform_t *self, u64 id);
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
                                      ocrParamList_t *instanceArg);
    void (*initialize)(struct _ocrCommPlatformFactory_t *factory, ocrCommPlatform_t * self,
                       ocrParamList_t *instanceArg);

    /**
     * @brief comm-platform factory destructor
     * @param factory       Pointer to the factory to destroy.
     */
    void (*destruct)(struct _ocrCommPlatformFactory_t *factory);

    ocrCommPlatformFcts_t platformFcts; /**< Function pointers created instances should use */
} ocrCommPlatformFactory_t;

void initializeCommPlatformOcr(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * self, ocrParamList_t *perInstance);

#endif /* __OCR_COMM_PLATFORM_H__ */
