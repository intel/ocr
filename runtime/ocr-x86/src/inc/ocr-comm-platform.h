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
     * structures to receive incoming messages. This indicates how
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
     * @brief Non-blocking check for incoming messages
     *
     * This function checks for ANY incoming message
     * @param[in] self        Pointer to this comm-platform
     * @param[in/out] msg     Returns a pointer to an ocrPolicyMsg_t. On input,
     *                        if *msg is non-NULL, this is a "hint" to the
     *                        comm-platform that it can use the provided buffer
     *                        for the response message. There is, however
     *                        no obligation. If *msg is non NULL on input,
     *                        bufferSize contains the size of the "hint" buffer
     *                        specified in the same way as for sendMessage (in
     *                        this case though, the actual size of the message
     *                        is irrelevant).
     *                        The comm-platform will *NEVER* free the input
     *                        buffer passed in. It is up to the caller to
     *                        determine if the comm-platform used the buffer
     *                        (if *msg on output is the same as *msg on input).
     *                        If on output *msg is not the same as the one
     *                        passed in, the output *msg will have been created
     *                        by the comm-platform and the caller needs to call
     *                        destructMessage once the buffer is done being used.
     *                        Otherwise (ie: *msg is the same on input and
     *                        output), destructMessage should not be called
     *                        If *msg is NULL on return, check return code
     *                        to see if there was an error or if no messages
     *                        were available
     * @param[in/out] bufferSize On input, contains the size of the hint
     *                        buffer if applicable. On output, contains the
     *                        size of the message returned (same format as
     *                        for sendMessage).
     * @param[in] properties  Unused for now
     * @param[out] mask       Mask of the returned message (may always be 0)
     * @return
     *     - #POLL_NO_MESSAGE: 0 messages found
     *     - 0: One message found and returned
     *     - #POLL_MORE_MESSAGE: 1 message found and returned but more available
     *     - <val> & #POLL_ERR_MASK: error code or 0 if all went well
     */
    u8 (*pollMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t **msg,
                      u64* bufferSize, u32 properties, u32 *mask);

    /**
     * @brief Blocking check for incoming messages
     *
     * This function checks for ANY incoming message
     * See pollMessage() for a detailed explanation of the parameters
     * @param[in] self        Pointer to this comm-platform
     * @param[in/out] msg     Pointer to the message returned
     * @param[in/out] bufferSize Size of the message returned
     * @param[in] properties  Unused for now
     * @param[out] mask       Mask of the returned message (may always be 0)
     * @return 0 on success and a non-zero error code
     */
    u8 (*waitMessage)(struct _ocrCommPlatform_t *self, struct _ocrPolicyMsg_t **msg,
                      u64 *bufferSize, u32 properties, u32 *mask);

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

/**
 * @brief Base initializer for comm-platforms
 */
void initializeCommPlatformOcr(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * self, ocrParamList_t *perInstance);

/****************************************************/
/* UTILITY FUNCTIONS                                */
/****************************************************/

typedef enum {
    MARSHALL_FULL_COPY,
    MARSHALL_APPEND,
    MARSHALL_ADDL
} ocrMarshallMode_t;

/**
 * @brief Gets the effective size of the msg that
 * needs to be sent
 *
 * This size includes the actual payload + whatever
 * additional space is needed to store variable-size
 * arguments such as GUIDs, funcName etc.
 *
 * @param[in] msg               Message to analyze
 * @param[out] fullSize         Full size of the message (bytes)
 * @param[out] marshalledSize   Additional data that needs to be
 *                              fetched/marshalled (included in fullSize) (bytes)
 *
 * This call can be used on both a ocrPolicyMsg_t or a buffer
 * containing a marshalled ocrPolicyMsg_t. This will set the 'size'
 * field in the ocrPolicyMsg_t to fullSize - marshalledSize
 *
 * @return 0 on success and a non-zero error code
 */
u8 ocrCommPlatformGetMsgSize(struct _ocrPolicyMsg_t* msg, u64 *fullSize,
                             u64* marshalledSize);

/**
 * @brief Marshall everything needed to send the message over to a PD that
 * does not share the same address space
 *
 * This function performs one of three operations depending on the
 * mode:
 *   - MARSHALL_FULL_COPY: Fully copy and marshall msg into buffer. In
 *                         this case buffer and msg do not overlap. The
 *                         caller also ensures that buffer is at least
 *                         big enough to contain fullSize bytes as
 *                         returned by ocrCommPlatformGetMsgSize. Assumes
 *                         that msg->size is properly set to (fullSize - marshalledSize)
 *   - MARSHALL_APPEND:    In this case (u64)buffer == (u64)msg and
 *                         marshalled values will be appended to the
 *                         end of the useful portion of msg. The caller
 *                         ensures that buffer is at least big enough to
 *                         contain fullSize bytes as returned by
 *                         ocrCommPlatformGetMsgSize
 *   - MARSHALL_ADDL:      Marshall only neede portions (not already in msg)
 *                         into buffer. The caller ensures that buffer is
 *                         at least big enough to contain marshalledSize
 *                         as returned by ocrCommPlatformGetMsgSize
 *
 * Note that in the case of MARSHALL_APPEND and MARSHALL_ADDL, the
 * msg itself is modified to encode offsets and positions for the marshalled
 * values so that it can be unmarshalled by ocrCommPlatformUnMarshallMsg. In
 * all cases, size is properly set (for FULL_COPY and APPEND to fullSize and
 * for ADDL to (fullSize - marshalledSize))
 *
 * @param[in/out] msg      Message to marshall from
 * @param[in/out] buffer   Buffer to marshall to
 * @param[in] mode         One of MARSHALL_FULL_COPY, MARSHALL_APPEND, MARSHALL_ADDL
 * @return 0 on success and a non-zero error code
 */
u8 ocrCommPlatformMarshallMsg(struct _ocrPolicyMsg_t* msg, u8* buffer, ocrMarshallMode_t mode);

/**
 * @brief Performs the opposite operation to ocrCommPlatformMarshallMsg
 *
 * The inputs are the mainBuffer (containing the header of the messsage and
 * an optional additional buffer (obtained using the MARSHALL_ADDL mode).
 *
 * The significance of the modes here are:
 *   - MARSHALL_FULL_COPY: In this case, none of the buffers (mainBuffer,
 *                         addlBuffer, msg) overlap. The entire content
 *                         of the message and any additional structure
 *                         will be put in msg. The caller ensures that
 *                         the actual allocated size of msg is at least
 *                         fullSize bytes (as returned by ocrCommPlatformGetMsgSize)
 *   - MARSHALL_APPEND:    In this case, (u64)msg == (u64)mainBuffer. In
 *                         most cases, this will only fix-up pointers but
 *                         may copy from addlBuffer into mainBuffer. For
 *                         example, the mashalling could have been
 *                         done using MARSHALL_ADDL and unmarshalling is
 *                         done with MARSHALL_APPEND.
 *                         The caller ensures that the actual allocated
 *                         size of msg is at least fullSize bytes (as
 *                         returned by ocrCommPlatformGetMsgSize)
 *   - MARSHALL_ADDL:      Marshall the common part of the message into msg
 *                         and use pdMalloc for marshalledSize bytes (one
 *                         or more chunks totalling marshalledSize bytes as
 *                         returned by ocrCommPlatformGetMsgSize)
 *
 * @param[in] mainBuffer   Buffer containing the common part of the message
 * @param[in] addlBuffer   Optional addtional buffer (if message was marshalled
 *                         using MARSHALL_ADDL). NULL otherwise (do not pass in
 *                         garbage!). It is assumed to have marshalledSize of useful
 *                         content
 * @param[out] msg         Message to create from mainBuffer and addlBuffer
 * @param[in] mode         One of MARSHALL_FULL_COPY, MARSHALL_APPEND, MARSHALL_ADDL
 *
 * @return 0 on success and a non-zero error code
 */
u8 ocrCommPlatformUnMarshallMsg(u8* mainBuffer, u8* addlBuffer,
                                struct _ocrPolicyMsg_t* msg, ocrMarshallMode_t mode);

#endif /* __OCR_COMM_PLATFORM_H__ */



