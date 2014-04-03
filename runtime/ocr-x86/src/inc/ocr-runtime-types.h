/**
 * @brief Basic types used throughout the OCR library (runtime only types)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_RUNTIME_TYPES_H__
#define __OCR_RUNTIME_TYPES_H__

#include "ocr-types.h"

#include "utils/profiler/profiler.h"

/**
 * @brief Memory region "tags"
 *
 * A tag indicates the type of the memory region in the
 * mem-target or mem-platform. Other tags may be added
 * as needed.
 * @see chunk() tag() queryTag()
 * @note Must be sequential and have MAX_TAG for rangeTracker to work
 */
typedef enum {
    RESERVED_TAG   = 0, /**< Non-usable chunk of memory */
    NON_USER_TAG    = 1, /**< Regions that are non user managed (text, bss for example */
    USER_FREE_TAG   = 2, /**< Regions that are free to be used by higher levels */
    USER_USED_TAG   = 3, /**< Regions that the higher levels have used */
    MAX_TAG         = 4  /**< Marker of the end of the tags */
} ocrMemoryTag_t;

/**
 * @brief "Capabilities" for comp-target and comp-platform
 *
 * Computation resources can have capabilities that allow minimal
 * support for heterogeneous infrastructure.
 */
typedef enum {
    COMP_CAPABILITY = 0x1, /**< Node is capable of computation */
    COMM_CAPABILITY = 0x2, /**< Node is capable of communicating with other nodes */
    MAX_CAPABILITY  = 0x3  /**< OR of all the previous capabilities */
} ocrComputeCapability_t;

/**
 * @brief Properties for the sendMessage calls
 * indicating the usage model of the call by the caller
 *
 * These flags allow the backing comm-platform to make
 * assumptions about the message being passed in. In
 * particular, the comm-platform can make assumptions
 * about whether the storage for a message is kept around
 * after the call
 */
typedef enum {
    TWOWAY_MSG_PROP          = 0x1, /**< A "response" is expected for
                                     * this message */
    PERSIST_MSG_PROP         = 0x2, /**< The input message is guaranteed to be
                                     * valid until *after* a successful poll/wait */
    PENDING_MSG_PROP         = 0x4,  /*message not processed yet*/
    PRIO1_MSG_PROP           = 0x100, /**< Lowest priority message */
    PRIO2_MSG_PROP           = 0x200, /**< Higher priority message */
    PRIO3_MSG_PROP           = 0x400, /**< Highest priority message */
} ocrMsgBehaviorProp_t;

/**
 * @brief Status of messages at the comm-platform level
 *
 * @note Not all implementations support all states. The mandatory
 * supported ones are marked. Others are present mostly
 * for optimization reasons
 */
typedef enum {
    MSG_NORMAL   = 0x00100, /**< (MANDATORY) The message is normal (no further details) */
    MSG_ERR      = 0x10100, /**< (MANDATORY) An error occured sending the message */
    MSG_ACCEPTED = 0x00200, /**< The message has been accepted by the comm-platform */
    MSG_SEND_ERR = 0x10400, /**< THe message had an error being sent */
    MSG_SEND_OK  = 0x00400, /**< The message has been sent successfully */
    MSG_RECV_OK  = 0x00800, /**< The message has been received by the target successfully */
    MSG_RECV_ERR = 0x10800, /**< The message had an error being received by the target */
} ocrMsgStatus_t;

/**
 * @brief Status of the message handle
 *
 * @note Not all implementations support all states. The
 * mandatory supported ones are marked. Others are present
 * mostly for optimization reasons
 */
typedef enum {
    HDL_NORMAL        = 0x00100, /**< (MANDATORY) No error conditions */
    HDL_ERR           = 0x10100, /**< (MANDATORY) Unspecified error */
    HDL_SEND_ERR      = 0x10200, /**< Outgoing message could not be sent */
    HDL_SEND_OK       = 0x00200, /**< Outgoing message was sent properly */
    HDL_SEND_RECV_OK  = 0x00201, /**< Outgoing message was acknowledged as received
                              * by the target */
    HDL_SEND_RECV_ERR = 0x10201, /**< Outgoing message was sent but an error
                              * occured on the receiving end */
    HDL_RECV_ERR      = 0x10400, /**< An error occured on the incomming response */
    HDL_RESPONSE_OK   = 0x00400  /**< (MANDATORY) Handle has a ready response */

} ocrMsgHandleStatus_t;
/**
 * @brief Types of data-blocks allocated by the runtime
 *
 * OCR provides support for "runtime" data-blocks which
 * may have more restrictive rules than regular data-blocks.
 *
 * @warn This functionality is not currently used and may be
 * stripped out but it is here in case we need it later
 */
typedef enum {
    USER_DBTYPE     = 0x1,
    RUNTIME_DBTYPE  = 0x2,
    MAX_DBTYPE      = 0x3
} ocrDataBlockType_t;

/**
 * @brief Type of memory allocated/unallocated
 * by MEM_ALLOC and MEM_UNALLOC
 *
 * The PD_MSG_MEM_ALLOC and PD_MSG_MEM_UNALLOC
 * messages allocate and de-allocate chunks of
 * memory. This indicates what these chunks of
 * memories are used for
 */
typedef enum {
    DB_MEMTYPE      = 0x1, /**< Memory used for a data-block */
    GUID_MEMTYPE    = 0x2, /**< Memory used for the metadata of an object */
    MAX_MEMTYPE     = 0x3
} ocrMemType_t;

/**
 * @brief Types of work "scheduled" by the runtime
 *
 * For now this is EDT but other type of computation
 * may be used (runtime EDTs, communication tasks, etc)
 *
 * @warn This functionality is not currently used and may be
 * stripped out but it is here in case we need it later
 */
typedef enum {
    EDT_WORKTYPE    = 0x1,
    MAX_WORKTYPE    = 0x2
} ocrWorkType_t;

typedef enum {
    CREATED_EDTSTATE    = 0x1, /**< EDT created; no dependence satisfied */
    PARTIAL_EDTSTATE    = 0x2, /**< EDT has at least one dependence that is satisfied */
    READY_EDTSTATE      = 0x3, /**< EDT has all dependences satisfied */
    RUNNING_EDTSTATE    = 0x4, /**< EDT is executing */
    REAPING_EDTSTATE    = 0x5  /**< EDT finished executing and is cleaning up */
} ocrEdtState_t;

/**
 * @brief Type of pop from workpiles.
 *
 * There are multiple ways to steal from a workpile
 * (traditionally the usual pop and also the steal).
 * This enum allows us to expand this in the future
 */
typedef enum {
    POP_WORKPOPTYPE     = 0x1,
    STEAL_WORKPOPTYPE   = 0x2,
    MAX_WORKPOPTYPE     = 0x3
} ocrWorkPopType_t;

/**
 * @brief Type of push to workpiles
 *
 * @see ocrWorkPopType_t
 */
typedef enum {
    PUSH_WORKPUSHTYPE   = 0x1,
    MAX_WORKPUSHTYPE    = 0x2
} ocrWorkPushType_t;

/**
 * @brief Type of worker
 *
 * This is used in workers as well as comp-* to determine what types
 * of workers the specific instance of the platform supports.
 * The distinction between SINGLE and MASTER is mostly to distinguish
 * between machines that require a single point of entry/exit (such as
 * a process with multiple threads that must fall back to only one thread
 * to properly exit) and others that can have multiple "main" entry points
 * (MPI nodes for example).
 */
typedef enum {
    SINGLE_WORKERTYPE   = 0x1, /**< Single worker (starts/stops by itself) */
    MASTER_WORKERTYPE   = 0x2, /**< Master worker (responsible for starting/stopping others */
    SLAVE_WORKERTYPE    = 0x3, /**< Slave worker (started/stopped by a master worker */
    MAX_WORKERTYPE      = 0x4
} ocrWorkerType_t;

typedef struct {
    ocrGuid_t guid;
    void* metaDataPtr;
} ocrFatGuid_t;

typedef enum {
    KIND_GUIDPROP       = 0x1, /**< Request kind of the GUID */
    WMETA_GUIDPROP      = 0x2, /**< Request the metadata of the GUID in write mode
                                * Also used when returning the GUID to indicate if
                                * the returned meta data can be written to */
    RMETA_GUIDPROP      = 0x4, /**< Request the metadata of the GUID in read mode */
    CMETA_GUIDPROP      = 0x8, /**< Indicates that the metadata returned is a copy (R/O
                                * and should be freed with pdFree) */
} ocrGuidInfoProp_t;

// REC: FIXME
// This is a placeholder for something that identifies a memory,
// a compute node and a policy domain.  Whatever else this becomes in the future, it
// includes the "engine index" in the low order eight bits.  Irrelevant for other
// platforms, this is needed by TG where it provides the block-based index for which
// processor or memory is identified:  0 == CE, 1-8 == XE; additional bits allow for
// a differnt number of XE's in other potential TG hardware family members.
typedef u64 ocrLocation_t;
#define ocrLocation_getEngineIndex(x) (x & 0xFFul)

/**
 * @brief Returned by the pollMessage function in
 * either the comp-target and comp-platform to indicate
 * that no message was available */
#define POLL_NO_MESSAGE   0x1
/**
 * @brief Indicates that a message was returned and available
 * and that more messages are available
 */
#define POLL_MORE_MESSAGE 0x2
/**
 * @brief AND the return code of pollMessage with this
 * mask to get any real error codes
 */
#define POLL_ERR_MASK     0xF0

/**
 * @brief Some useful macros
 */
#define RESULT_PROPAGATE(expression)            \
    do {                                        \
        u8 __result = (expression);             \
        if(__result) return (__result);         \
    } while(0);

#define RESULT_PROPAGATE2(expression, returnValue)  \
    do {                                            \
        u8 __result = (expression);                 \
        if(__result) return (returnValue);          \
    } while(0);

#define RESULT_PROPAGATE_PROF(expression)       \
    do {                                        \
        u8 __result = (expression);             \
        if(__result) RETURN_PROFILE(__result);  \
    } while(0);

#define RESULT_PROPAGATE2_PROF(expression, returnValue) \
    do {                                                \
        u8 __result = (expression);                     \
        if(__result) RETURN_PROFILE(returnValue);       \
    } while(0);

#endif /* __OCR_RUNTIME_TYPES_H__ */

