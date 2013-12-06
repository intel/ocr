/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_POLICY_DOMAIN_H_
#define OCR_POLICY_DOMAIN_H_

#include "ocr-allocator.h"
#include "ocr-comp-target.h"
#include "ocr-datablock.h"
#include "ocr-event.h"
#include "ocr-guid.h"
#include "ocr-mem-target.h"
#include "ocr-scheduler.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

#include "ocr-sys.h"
#include "ocr-task.h"
#include "ocr-tuning.h"
#include "ocr-types.h"
#include "ocr-worker.h"
#include "ocr-workpile.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListPolicyDomainFact_t {
    ocrParamList_t base;
} paramListPolicyDomainFact_t;

typedef struct _paramListPolicyDomainInst_t {
    ocrParamList_t base;
} paramListPolicyDomainInst_t;


/******************************************************/
/* OCR POLICY DOMAIN INTERFACE                        */
/******************************************************/

/**< Invalid message */
#define PD_MSG_INVAL            0

/* Bit structure for types of message:
 *     - Bottom 12 bits encode whether the message is about computation,
 *       memory, etc (001, 002, 004, ...)
 *     - Next 4 bits indicate the operation ID (sequential number 1, 2, 3...)
 *     - 0x100000 and 0x200000 indicate query/response
 */
/**< AND with this and if result non-null, memory related operation.
 * Generally, these will be DBs but it could be something else too
 */
#define PD_MSG_MEM_OP            0x001
/**< Create a memory area (allocate) */
#define PD_MSG_MEM_CREATE        0x1001
/**< Destroy a DB */
#define PD_MSG_MEM_DESTROY       0x2001


/**< AND with this and if result non-null, work/task related operation.
 * Generally, these will be EDTs but it could be something else too
 */
#define PD_MSG_WORK_OP           0x002
/**< Create an EDT */
#define PD_MSG_WORK_CREATE       0x1002
/**< Destroy an EDT */
#define PD_MSG_WORK_DESTROY      0x2002

/**< AND with this and if result non-null, EDT-template related operation */
#define PD_MSG_EDTTEMP_OP       0x004
/**< Create an EDT template */
#define PD_MSG_EDTTEMP_CREATE   0x1004
/**< Destroy an EDT template */
#define PD_MSG_EDTTEMP_DESTROY  0x2004

/**< AND with this and if result non-null, Event related operation */
#define PD_MSG_EVT_OP           0x008
/**< Create an event */
#define PD_MSG_EVT_CREATE       0x1008
/**< Destroy an event */
#define PD_MSG_EVT_DESTROY      0x2008
/**< Satisfy an event */
#define PD_MSG_EVT_SATISFY      0x3008

/**< AND with this and if result non-null, GUID related operations */
#define PD_MSG_GUID_OP          0x010
#define PD_MSG_GUID_CREATE      0x1010
#define PD_MSG_GUID_INFO        0x2010
#define PD_MSG_GUID_DESTROY     0x3010
// TODO: Add stuff about GUID reservation

/**< AND with this and if result non-null, GUID distribution related
 * operation (taking/giving EDTs, DBs, events, etc)
 */
#define PD_MSG_COMM_OP          0x020
/**< Request for a GUID (ie: the caller wants the callee
 * to give it the GUID(s) requested (pull model) */
#define PD_MSG_COMM_TAKE        0x1020
/**< Request for a GUID to be put (ie: the caller wants the
 * callee to accept the GUID(s) given (push model) */
#define PD_MSG_COMM_GIVE        0x2020

// Add more messages in here


/**< AND with this and if result non-null, low-level OS operation */
#define PD_MSG_SYS_OP           0x100
/**< Print operation */
#define PD_MSG_SYS_PRINT        0x1100
/**< Read operation */
#define PD_MSG_SYS_READ         0x2100
/**< Write operation */
#define PD_MSG_SYS_WRITE        0x3100
// TODO: Possibly add open, close, seek

/**< Shutdown operation (indicates the PD should
 * shut-down. Another call "finish" will actually
 * cause their destruction */
#define PD_MSG_SYS_SHUTDOWN     0x4100

/**< Finish operation (indicates the PD should
 * destroy itself. The existence of other PDs can
 * no longer be assumed */
#define PD_MSG_SYS_FINISH       0x5100

/**< And this to just get the type of the message without
 * the upper flags
 */
#define PD_MSG_TYPE_ONLY        0xFFFFF

#define PD_MSG_META_ONLY        0xFFF00000

/**< Defines that the message is a query (non-answered) */
#define PD_MSG_REQUEST          0x100000
/**< Defines that the message is a response */
#define PD_MSG_RESPONSE         0x200000
/**< Defines if the message requires a return message */
#define PD_MSG_REQ_RESPONSE     0x400000


#define PD_MSG_ARG_NAME_SUB(ID) _data_##ID
#define PD_MSG_STRUCT_NAME(ID) PD_MSG_ARG_NAME_SUB(ID)


#define PD_MSG_FIELD_FULL_SUB(ptr, type, field) ptr->args._data_##type.field
#define PD_MSG_FIELD_FULL(ptr, type, field) PD_MSG_FIELD_FULL_SUB((ptr), type, field)
#define PD_MSG_FIELD(field) PD_MSG_FIELD_FULL(PD_MSG, PD_TYPE, field)

struct _ocrPolicyDomain_t;

/**
 * @brief Structure describing a "message" that is used to communicate between
 * policy domains in an asynchronous manner
 *
 * Communication between policy domains is always assumed to be asynchronous
 * where the requester (source) does not execute the code of the remote
 * policy domain (it is therefore like a remote procedure call). Specific
 * implementations may choose a synchronous communication but asynchronous
 * communication is assumed.
 *
 * This class defines the "message" that will be sent between policy
 * domains. All messages to/from policy domains are encapsulated
 * in this format (even if synchronous)
 */
typedef struct _ocrPolicyMsg_t {
    u32 type;                 /**< Type of the message. Also includes if this
                               * is a request or a response */
    ocrGuid_t srcCompTarget;  /**< Source of the message
                              * (the compute target making the request */
    ocrGuid_t destCompTarget; /**< Destination of the message
                              * (the compute target processing the request) */
    u64 msgId;                /**< Implementation specific ID identifying
                              * this message (if required) */
    union {
        struct {
            ocrFatGuid_t guid;            /**< In/Out: GUID of the created
                                          * memory segment (usually a DB) */
            void* ptr;                    /**< Out: Address of created DB */
            u64 size;                     /**< In: Size of the created DB */
            ocrFatGuid_t affinity;        /**< In: Affinity group for the DB */
            u32 properties;               /**< In: Properties for creation */
            ocrDataBlockType_t dbType;    /**< Type of memory requested */
            ocrInDbAllocator_t allocator; /**< In: In-DB allocator */
        } PD_MSG_STRUCT_NAME(PD_MSG_MEM_CREATE);
        
        struct {
            ocrFatGuid_t guid; /**< In: GUID of the DB to destroy */
            u32 properties;    /**< In: properties for the destruction */
        } PD_MSG_STRUCT_NAME(PD_MSG_MEM_DESTROY);
        
        struct {
            ocrFatGuid_t guid;         /**< In/Out: GUID of the EDT/Work
                                        * to create */
            ocrFatGuid_t templateGuid; /**< In: GUID of the template to use */
            ocrFatGuid_t affinity;     /**< In: Affinity for this EDT */
            ocrFatGuid_t outputEvent;  /**< Out: If not NULL_GUID on input
                                       * will contain the event that will be
                                       * satisfied when this EDT completes */
            u64 *paramv;               /**< In: Parameters for this EDT */
            u32 paramc;                /**< In: Number of parameters */
            u32 depc;                  /**< In: Number of dependence slots */
            u32 properties;            /**< In: properties for the creation */
            ocrWorkType_t workType;    /**< In: Type of work to create */
        } PD_MSG_STRUCT_NAME(PD_MSG_WORK_CREATE);
        
        struct {
            ocrFatGuid_t guid; /**< In: GUID of the EDT to destroy */
            u32 properties;    /**< In: properties for the destruction */
        } PD_MSG_STRUCT_NAME(PD_MSG_WORK_DESTROY);
        
        struct {
            ocrFatGuid_t guid;     /**< In/Out: GUID of the EDT template */
            ocrEdt_t funcPtr;      /**< In: Function to execute for this EDT */
            u32 paramc;            /**< In: Number of parameters for EDT */
            u32 depc;              /**< In: Number of dependences for EDT */
            const char * funcName; /**< In: Debug help: user identifier */
        } PD_MSG_STRUCT_NAME(PD_MSG_EDTTEMP_CREATE);
        
        struct {
            ocrFatGuid_t guid; /**< In: GUID of the EDT template to destroy */
            u32 properties;    /**< In: properties for the destruction */
        } PD_MSG_STRUCT_NAME(PD_MSG_EDTTEMP_DESTROY);
        
        struct {
            ocrFatGuid_t guid;    /**< In/Out: GUID of the event to create */
            u32 properties;       /**< In: Properties for this creation */
            ocrEventTypes_t type; /**< Type of the event created */
        } PD_MSG_STRUCT_NAME(PD_MSG_EVT_CREATE);
        
        struct {
            ocrFatGuid_t guid; /**< In: GUID of the event to destroy */
            u32 properties;    /**< In: properties for the destruction */
        } PD_MSG_STRUCT_NAME(PD_MSG_EVT_DESTROY);
        
        struct {
            ocrFatGuid_t guid;    /**< In: GUID of the event to satisfy */
            ocrFatGuid_t payload; /**< In: GUID of the "payload" to satisfy the
                                  * event with (a DB usually) */
            u32 slot;             /**< In: Slot to satisfy the event on */
            u32 properties;       /**< In: Properties for the satisfaction */
        } PD_MSG_STRUCT_NAME(PD_MSG_EVT_SATISFY);
        
        struct {
            ocrFatGuid_t guid; /**< In/Out:
                               *  In: The metaDataPtr field contains the value
                               *  to associate with the GUID
                               *  Out: The guid field contains created GUID */
            ocrGuidKind kind;  /**< In: Kind of the GUID to create */
            u32 properties;    /**< In: Properties for the creation */
        } PD_MSG_STRUCT_NAME(PD_MSG_GUID_CREATE);

        struct {
            ocrFatGuid_t guid; /**< In/Out:
                                * In: The GUID to "deguidify" or the metaDataPtr to
                                * guidify
                                * Out: Fully resolved information
                                * value */
            ocrGuidKind kind; /**< Out: Contains the type of the GUID */
            u32 properties;   /**< In: Properties for the info */
        } PD_MSG_STRUCT_NAME(PD_MSG_GUID_INFO);
            
        struct {
            ocrFatGuid_t guid; /**< In: GUID to destroy */
            u32 properties;    /**< In: Properties for the destruction */
        } PD_MSG_STRUCT_NAME(PD_MSG_GUID_DESTROY);
        
        struct {
            ocrFatGuid_t *guids; /**< In/Out: GUID(s) of the work/DB/etc taken:
                                 * Input (optional): GUID(s) requested
                                 * Output: GUID(s) given to the caller
                                 * by the callee */
            u32 guidCount;       /**< In/Out: Number of GUID(s) in guids */
            u32 properties;      /**< In: properties for the take */
            ocrGuidKind type;    /**< In: Kind of GUIDs requested */
            // TODO: Add something about cost/choice heuristic
        } PD_MSG_STRUCT_NAME(PD_MSG_COMM_TAKE);
        struct {
            ocrFatGuid_t *guids; /**< In/Out: GUID(s) of the work/DB/etc given:
                                 * Input: GUID(s) the caller wants to hand-off
                                 * to the callee
                                 * Output (optional): GUID(s) NOT accepted
                                 * by callee */
            u32 guidCount;       /**< Number of GUID(s) in guids */
            u32 properties;      /**< In: properties for the give */
            ocrGuidKind type;    /**< In: Kind of GUIDs given */
            // TODO: Do we need something about cost/choice heuristic here
        } PD_MSG_STRUCT_NAME(PD_MSG_COMM_GIVE);

        struct {
            const char* buffer;  /**< In: Character string to print */
            u64 length;          /**< In: Length to print */
            u32 properties;      /**< In: Properties for the print */
        } PD_MSG_STRUCT_NAME(PD_MSG_SYS_PRINT);

        struct {
            char* buffer;       /**< In/Out: Buffer to read into */
            u64 length;         /**< In/Out: On input contains maximum length
                                 * of buffer, on output, contains length read */
            u64 inputId;        /**< In: Identifier for where to read from */
            u32 properties;     /**< In: Properties for the read */
        } PD_MSG_STRUCT_NAME(PD_MSG_SYS_READ);
        struct {
            const char* buffer; /**< In: Buffer to write */
            u64 length;         /**< In/Out: Number of bytes to write. On return contains
                                 * bytes NOT written */
            u64 outputId;       /**< In: Identifier for where to write to */
            u32 properties;     /**< In: Properties for the write */
        } PD_MSG_STRUCT_NAME(PD_MSG_SYS_WRITE);
    } args;
} ocrPolicyMsg_t;


/**
 * @brief A policy domain is OCR's way of dividing up the runtime in scalable
 * chunks
 *
 * A policy domain is considered a 'synchronous' section of the runtime. In
 * other words, if a given worker makes a call into its policy domain, that call
 * will be processed synchronously by that worker (and therefore by the
 * compute target and platform running the worker) as long as it stays within
 * the policy domain. When the call requires processing outside the policy
 * domain, an asynchronous one-way 'message' will be sent to the other policy
 * domain (this is similar to a remote procedure call). Implementation
 * would determine if the RPC call is actually executed asynchronously by
 * another worker or executed synchronously by the same worker but it will
 * always be treated as an asynchronous call. Since the user API is
 * synchronous, if no response is needed, the call into the policy domain
 * can then return to the caller. However, if a response is required, a
 * worker specific function is called informing it that it needs to
 * 'wait-for-network' (or something like that). The exact implementation
 * will determine whether the worker halts or does something else.
 *
 * Each policy domain contains the following:
 *     - 0 or more 'schedulers' which both schedule tasks and place data
 *     - 0 or more 'workers' on which EDTs are executed
 *     - 0 or more 'target computes' on which 'workers' are mapped and run
 *     - 0 or more 'workpiles' from which EDTs are taken to be scheduled
 *       on the workers. Note that the exact content of workpiles is
 *       left up to the scheduler managing them
 *     - 0 or more 'allocators' from which DBs are allocated/freed
 *     - 0 or more 'target memories' which are managed by the
 *       allocators
 *
 * A policy domain will directly respond to requests emanating from user
 * code to:
 *     - create EDTs (using ocrEdtCreate())
 *     - create DBs (using ocrDbCreate())
 *     - create GUIDs
 *
 * It will also respond to requests from the runtime to:
 *     - destroy and reallocate a data-block (callback stemming from user
 *       actions on data-blocks using ocrDbDestroy())
 *     - destroy an EDT (callback stemming from user actions on EDTs
 *       using ocrEdtDestroy())
 *     - partial or complete satisfaction of dependences (callback
 *       stemming from user actions on EDTs using ocrEventSatisfy())
 *     - take EDTs stemming from idling workers for example
 *
 * Finally, it will respond to requests from other policy domains to:
 *     - take (steal) EDTs and DBs (data load-balancing)
 */
typedef struct _ocrPolicyDomain_t {
    ocrGuid_t guid;                             /**< GUID for this policy */

    u64 schedulerCount;                         /**< Number of schedulers */
    u64 workerCount;                            /**< Number of workers */
    u64 computeCount;                           /**< Number of target computate
                                                 * nodes */
    u64 workpileCount;                          /**< Number of workpiles */
    u64 allocatorCount;                         /**< Number of allocators */
    u64 memoryCount;                            /**< Number of target memory
                                                 * nodes */

    ocrScheduler_t  ** schedulers;              /**< All the schedulers */
    ocrWorker_t     ** workers;                 /**< All the workers */
    ocrCompTarget_t ** computes;                /**< All the target compute
                                                 * nodes */
    ocrWorkpile_t   ** workpiles;               /**< All the workpiles */
    ocrAllocator_t  ** allocators;              /**< All the allocators */
    ocrMemTarget_t  ** memories;                /**< All the target memory */

    ocrTaskFactory_t  * taskFactory;            /**< Factory to produce tasks
                                                 * (EDTs) */
    
    ocrTaskTemplateFactory_t  * taskTemplateFactory; /**< Factory to produce
                                                      * task templates */
    ocrDataBlockFactory_t * dbFactory;          /**< Factory to produce
                                                 * data-blocks */
    ocrEventFactory_t * eventFactory;           /**< Factory to produce events*/

    ocrGuidProvider_t *guidProvider;            /**< GUID generator */

    ocrSys_t *sysProvider;                      /**< Low-level system functions */

#ifdef OCR_ENABLE_STATISTICS
    ocrStats_t *statsObject;                    /**< Statistics object */
#endif

    // TODO: What to do about this?
    ocrCost_t *costFunction; /**< Cost function used to determine
                              * what to schedule/steal/take/etc.
                              * Currently a placeholder for future
                              * objective driven scheduling */

    /**
     * @brief Destroys (and frees any associated memory) this
     * policy domain
     *
     * Call when the policy domain has stopped executing to free
     * any remaining memory.
     *
     * @param self                This policy domain
     */
    void (*destruct)(struct _ocrPolicyDomain_t *self);

    /**
     * @brief Starts this policy domain
     *
     * This starts the portion of OCR that manages the resources contained
     * in this policy domain.
     *
     * @param self                This policy domain
     */
    void (*start)(struct _ocrPolicyDomain_t *self);

    /**
     * @brief Stops this policy domain
     *
     * This stops the portion of OCR that manages the resources
     * contained in this policy domain. The policy domain will also stop
     * responding to requests from other policy domains.
     *
     * @param self                This policy domain
     */
    void (*stop)(struct _ocrPolicyDomain_t *self);

    /**
     * @brief Finish the execution of the policy domain
     *
     * Ask the policy domain to wrap up currently executing
     * task and shutdown workers. This is typically something
     * ocrShutdown would call.
     *
     * @param self                This policy domain
     */
    void (*finish)(struct _ocrPolicyDomain_t *self);

    /**
     * @brief Requests for the handling of the request msg
     *
     * This function can be called either by user code (for when the user code
     * requires runtime services) or internally by the runtime.
     *
     * All code executed by processMessage until it reaches a sendMessage
     * is executed synchronously and inside the same address space.
     *
     * @param[in]     self       This policy domain
     * @param[in/out] msg        Message to process
     * @param[in]     isBlocking True if the processing of the message
     *                           need to complete before returning
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*processMessage)(struct _ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg,
                         u8 isBlocking);

    /**
     * @brief Called to request that the message 'msg' be sent outside
     * this policy domain
     *
     * If a policy domain cannot handle a message locally, it can
     * forward it to another compute target (belonging to another
     * policy domain). This call may lead to asynchronous
     * execution but ifBlocking is true, the call will not return
     * until a response (if required) is received. Note that
     * this does not mean that the compute target blocks (this is
     * implementation dependent)
     *
     * @param[in]     self       This policy domain
     * @param[in/out] msg        Message to send
     * @param[in]     isBlocking True if the response needs to be received
     *                           prior to returning
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*sendMessage)(struct _ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg,
                      u8 isBlocking);

    /**
     * @brief Called when a request is received from an underlying
     * compute target (never called by user code) to process the message
     *
     * @param[in]     self       This policy domain
     * @param[in/out] msg        Message to process
     * @return 0 on success and a non-zero value on failure
     */
    u8 (*receiveMessage)(struct _ocrPolicyDomain_t *self, ocrPolicyMsg_t *msg);
    
#ifdef OCR_ENABLE_STATISTICS
    ocrStats_t* (*getStats)(struct _ocrPolicyDomain_t *self);
#endif

    ocrSys_t* (*getSys)(struct _ocrPolicyDomain_t *self);
    
    struct _ocrCompTarget_t** neighbors;
    u64 neighborCount;
    
} ocrPolicyDomain_t;

/****************************************************/
/* OCR POLICY DOMAIN FACTORY                        */
/****************************************************/

typedef struct _ocrPolicyDomainFactory_t {
    /**
     * @brief Create a policy domain
     *
     * Allocates the required space for the policy domain
     * based on the counts passed as arguments. The 'schedulers',
     * 'workers', 'computes', 'workpiles', 'allocators' and 'memories'
     * data-structures must then be properly filled
     *
     * @param factory             This policy domain factory
     * @param configuration       An optional configuration
     * @param schedulerCount      The number of schedulers
     * @param workerCount         The number of workers
     * @param computeCount        The number of compute targets
     * @param workpileCount       The number of workpiles
     * @param allocatorCount      The number of allocators
     * @param memoryCount         The number of memory targets
     * @param taskFactory         The factory to use to generate EDTs
     * @param taskTemplateFactory The factory to use to generate EDTs templates
     * @param dbFactory           The factory to use to generate DBs
     * @param eventFactory        The factory to use to generate events
     * @param contextFactory      The factory to use to generate context
     * @param guidProvider        The provider of GUIDs for this policy domain
     * @param lockFactory         The factory to use to generate locks
     * @param atomicFactory       The factory to use to generate atomics
     * @param queueFactory        The factory to use to generate queues
     * @param costFunction        The cost function used by this policy domain
     */

    ocrPolicyDomain_t * (*instantiate)
        (struct _ocrPolicyDomainFactory_t *factory, u64 schedulerCount,
         u64 workerCount, u64 computeCount, u64 workpileCount,
         u64 allocatorCount, u64 memoryCount, ocrTaskFactory_t *taskFactory,
         ocrTaskTemplateFactory_t *taskTemplateFactory,
         ocrDataBlockFactory_t *dbFactory, ocrEventFactory_t *eventFactory,
         ocrGuidProvider_t *guidProvider, ocrSys_t *sysProvider,
#ifdef OCR_ENABLE_STATISTICS
         ocrStats_t *statsObject,
#endif
         ocrCost_t *costFunction, ocrParamList_t *perInstance);

    void (*destruct)(struct _ocrPolicyDomainFactory_t * factory);
} ocrPolicyDomainFactory_t;

#define __GUID_END_MARKER__
#include "ocr-guid-end.h"
#undef __GUID_END_MARKER__

#endif /* OCR_POLICY_DOMAIN_H_ */
