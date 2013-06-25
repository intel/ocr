/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef OCR_POLICY_DOMAIN_H_
#define OCR_POLICY_DOMAIN_H_

#include "ocr-types.h"
#include "ocr-guid.h"
#include "ocr-scheduler.h"
#include "ocr-comp-target.h"
#include "ocr-worker.h"
#include "ocr-allocator.h"
#include "ocr-mem-target.h"
#include "ocr-datablock.h"
#include "ocr-sync.h"
#include "ocr-mappable.h"
#include "ocr-event.h"
#include "ocr-task.h"
#include "ocr-tuning.h"
#include "ocr-workpile.h"

/******************************************************/
/* OCR POLICY DOMAIN INTERFACE                        */
/******************************************************/

typedef enum {
    PD_MSG_INVAL       = 0, /**< Invalid message */
    PD_MSG_EDT_READY   = 1, /**< An EDT is now fully "satisfied" */
    PD_MSG_EDT_SATISFY = 2, /**< Partial EDT satisfaction */
    PD_MSG_DB_DESTROY  = 3, /**< A DB destruction has been requested */
    PD_MSG_EDT_TAKE    = 4, /**< Take EDTs (children of the PD make this call) */
    PD_MSG_DB_TAKE     = 5, /**< Take DBs (children of the PD make this call) */
    PD_MSG_EDT_STEAL   = 6, /**< Steal EDTs (from non-children PD) */
    PD_MSG_DB_STEAL    = 7, /**< Steal DBs (from non-children PD) */
} ocrPolicyMsgType_t;

/**
 * @brief Structure describing a "message" that is used to communicate between
 * policy domains in an asynchronous manner
 *
 * Policy domains can communicate between each other using either
 * a synchronous method where the requester (source) policy domain executes
 * the code of the requestee (destination) policy domain directly or
 * asynchronously where the source domain sends the equivalent of a message
 * to the destination domain and then `waits' for a response. The first
 * way requires synchronization and homogeneous runtime cores and is the
 * most natural model on x86 while the second model is more natural in a
 * slave-master framework where the slaves are not allowed to execute
 * code written for the master.
 *
 * This struct is meant to be extended (either one per implementation
 * or optionally one per implementation and type).
 *
 * Note that this structure is also used for synchronous messages
 * so that the caller knows who finally responded to its request.
 */
typedef struct _ocrPolicyCtx_t {
    ocrGuid_t sourcePD;      /**< Source policy domain */
    ocrGuid_t sourceObj;     /**< Source object (worker for example) in the source PD */
    u64       sourceId;      /**< Internal ID to the source PD */
    ocrGuid_t destPD;        /**< Destination policy domain (after all eventual hops) */
    ocrGuid_t destObj;       /**< Responding object (after all eventual hops) */
    ocrPolicyMsgType_t type; /**< Type of message */

    void (*destruct)(struct _ocrPolicyCtx_t *self);
} ocrPolicyCtx_t;


typedef struct _ocrPolicyCtxFactory_t {
    ocrPolicyCtx_t * (*instantiate)(struct _ocrPolicyCtxFactory_t *factory,
                                    void* perTypeConfig, void* perInstanceConfig);
    void (*destruct)(struct _ocrPolicyCtx_t *self);
} ocrPolicyCtxFactory_t;

/**
 * @brief A policy domain is OCR's way of dividing up the runtime in scalable
 * chunks
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
    ocrMappable_t module;
    ocrGuid_t guid;                             /**< GUID for this policy */

    u32 schedulerCount;                         /**< Number of schedulers */
    u32 workerCount;                            /**< Number of workers */
    u32 computeCount;                           /**< Number of target computate nodes */
    u32 workpileCount;                          /**< Number of workpiles */
    u32 allocatorCount;                         /**< Number of allocators */
    u32 memoryCount;                            /**< Number of target memory nodes */

    ocrScheduler_t  ** schedulers;              /**< All the schedulers */
    ocrWorker_t     ** workers;                 /**< All the workers */
    ocrCompTarget_t ** computes;                /**< All the target compute nodes */
    ocrWorkpile_t   ** workpiles;               /**< All the workpiles */
    ocrAllocator_t  ** allocators;              /**< All the allocators */
    ocrMemTarget_t  ** memories;                /**< All the target memory nodes */

    ocrTaskFactory_t  * taskFactory;            /**< Factory to produce tasks (EDTs) in this policy domain */
    ocrTaskTemplateFactory_t  * taskTemplateFactory; /**< Factory to produce task templates in this policy domain */
    ocrDataBlockFactory_t * dbFactory;          /**< Factory to produce data-blocks in this policy domain */
    ocrEventFactory_t * eventFactory;           /**< Factory to produce events in this policy domain */

    ocrPolicyCtxFactory_t * contextFactory;     /**< Factory to produce the contexts (used for communicating
                                                 * between policy domains) */

    ocrGuidProvider_t *guidProvider;            /**< GUID generator for this policy domain */

    ocrLockFactory_t *lockFactory;              /**< Factory for locks */
    ocrAtomic64Factory_t *atomicFactory;        /**< Factory for atomics */

    ocrCost_t *costFunction;                    /**< Cost function used to determine
                                                 * what to schedule/steal/take/etc.
                                                 * Currently a placeholder for future
                                                 * objective driven scheduling */

    /**
     * @brief Create a policy domain
     *
     * Allocates the required space for the policy domain
     * based on the counts passed as arguments. The 'schedulers',
     * 'workers', 'computes', 'workpiles', 'allocators' and 'memories'
     * data-structures must then be properly filled
     *
     * @param self                This policy
     * @param configuration     An optional configuration
     * @param schedulerCount    The number of schedulers
     * @param workerCount       The number of workers
     * @param computeCount      The number of compute targets
     * @param workpileCount     The number of workpiles
     * @param allocatorCount    The number of allocators
     * @param memoryCount       The number of memory targets
     * @param taskFactory       The factory to use to generate EDTs
     * @param dbFactory         The factory to use to generate DBs
     * @param eventFactory      The factory to use to generate events
     * @param contextFactory    The factory to use to generate context
     * @param guidProvider      The provider of GUIDs for this policy domain
     * @param costFunction      The cost function used by this policy domain
     */
    void (*create)(struct _ocrPolicyDomain_t *self, void *configuration,
                   u32 schedulerCount, u32 workerCount, u32 computeCount,
                   u32 workpileCount, u32 allocatorCount, u32 memoryCount,
                   ocrTaskFactory_t *taskFactory, ocrDataBlockFactory_t *dbFactory,
                   ocrEventFactory_t *eventFactory, ocrPolicyCtxFactory_t
                   *contextFactory, ocrCost_t *costFunction);

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

    // TODO: Do we need a finish? What was it for


    /**
     * @brief Request the allocation of memory (a data-block)
     *
     * This call will be triggered by user code when a data-block
     * needs to be allocated
     *
     * @param self                This policy domain
     * @param guid              Contains the DB GUID on return for synchronous
     *                          calls
     * @param size              Size of the DB requested
     * @param hint              Hint concerning where to allocate
     * @param context           Context for this call. This will be updated
     *                          as the call progresses (with the destination
     *                          information for example)
     *
     * If this policy domain implements synchronous calls, this call will only
     * return once the entire call has been serviced. In the asynchronous case,
     * this call will return a value indicating asynchronous processing and the
     * policy domain will be notified when the call returns.
     *
     * @return:
     *     - 0 if the call completed successfully synchronously
     *     - 255 if the call is being processed asynchronously
     *     - TODO
     */
    u8 (*allocateDb)(struct _ocrPolicyDomain_t *self, ocrGuid_t *guid, u64 size,
                     ocrHint_t *hint, ocrPolicyCtx_t *context);

    /**
     * @brief Request the creation of a task metadata (EDT)
     *
     * This call will be triggered by user code when an EDT
     * is created
     *
     * @todo Improve description to be more like allocateDb
     *
     * @todo Add something about templates here and potentially
     * known dependences so that it can be optimally placed
     */
    u8 (*createEdt)(struct _ocrPolicyDomain_t *self, ocrGuid_t *guid, ocrHint_t *hint,
                    ocrPolicyCtx_t *context);

    /**
     * @brief Inform the policy domain of an event that does not require any
     * further processing
     *
     * These events are informational to the policy domain (runtime) and include
     * things like dependence satisfaction, destruction of a DB, etc. The context
     * will encode the type of event and any additional information
     *
     * This call will return immediately (whether in asynchronous or
     * synchronous mode)
     */
    void (*inform)(struct _ocrPolicyDomain_t *self, ocrGuid_t obj,
                   const ocrPolicyCtx_t *context);

    /**
     * @brief Gets a GUID and associates the value 'val' with
     * it
     *
     * This is used by the runtime to obtain new GUIDs
     *
     * @todo Write description, behaves as other functions
     */
    u8 (*getGuid)(struct _ocrPolicyDomain_t *self, ocrGuid_t *guid, u64 val,
                  ocrGuidKind type);

    u8 (*getInfoForGuid)(struct _ocrPolicyDomain_t *self, ocrGuid_t guid, u64* val,
                         ocrGuidKind* type);

    /**
     * @brief Take one or more EDTs to execute
     *
     * This call is called by either workers within this PD that need work or by
     * other PDs that are trying to "steal" work from here
     *
     * On a synchronous return, the number of EDTs taken and a pointer
     * to an array of GUIDs for them will be returned. On an asynchronous return,
     * count will be 0 and the GUIDs pointer will be invalid; the information
     * will have to come from the context
     *
     * @param self        This policy domain
     * @param cost      An optional cost function provided by the taker
     * @param count     On return contains the number of EDTs taken (synchronous calls)
     * @param edts      On return contains the EDTs taken (synchronous calls)
     * @param context   Context for this call
     *
     * @return:
     *     - 0 if the call completed successfully synchronously
     *     - 255 if the call is being processed asynchronously
     *     - TODO
     */
    u8 (*takeEdt)(struct _ocrPolicyDomain_t *self, ocrCost_t *cost, u32 *count,
                  ocrGuid_t *edts, ocrPolicyCtx_t *context);

    /**
     * @brief Same as takeEdt but for data-blocks
     *
     * @todo Add full description
     */
    u8 (*takeDb)(struct _ocrPolicyDomain_t *self, ocrCost_t *cost, u32 *count,
                 ocrGuid_t *dbs, ocrPolicyCtx_t *context);

    /**
     * @brief A function called by other policy domains to hand-off
     * EDTs; this is in essence, the opposite of takeEdt
     *
     * @todo Give more detail
     * @todo Is this needed?
     */
    u8 (*giveEdt)(struct _ocrPolicyDomain_t *self, u32 count, ocrGuid_t *edts,
                  ocrPolicyCtx_t *context);

    /**
     * @brief Same as giveEdt() but for data-blocks
     */
    u8 (*giveDb)(struct _ocrPolicyDomain_t *self, u32 count, ocrGuid_t *dbs,
                 ocrPolicyCtx_t *context);

    /**
     * @brief Called for asynchronous calls when the call has been
     * processed.
     *
     * @param self        This policy domain
     * @param context   Context for the call (indicating what it is a response to)
     */
    void (*processResponse)(struct _ocrPolicyDomain_t *self, ocrPolicyCtx_t *context);

    ocrLock_t* (*getLock)(struct _ocrPolicyDomain_t *self);
    ocrAtomic64_t* (*getAtomic64)(struct _ocrPolicyDomain_t *self);

    struct _ocrPolicyDomain_t** neighbors;
    u32 neighborCount;

} ocrPolicyDomain_t;

// /**!
//  * Default destructor for ocr policy domain
//  */
// void ocr_policy_domain_destruct(ocrPolicyDomain_t * policy);

// /******************************************************/
// /* OCR POLICY DOMAIN KINDS AND CONSTRUCTORS           */
// /******************************************************/

// typedef enum ocr_policy_domain_kind_enum {
//     OCR_POLICY_HC = 1,
//     OCR_POLICY_XE = 2,
//     OCR_POLICY_CE = 3,
//     OCR_POLICY_MASTERED_CE = 4,
//     OCR_PLACE_POLICY = 5,
//     OCR_LEAF_PLACE_POLICY = 6,
//     OCR_MASTERED_LEAF_PLACE_POLICY = 7
// } ocr_policy_domain_kind;

// ocrPolicyDomain_t * newPolicyDomain(ocr_policy_domain_kind policyType,
//                                 u64 workpileCount,
//                                 u64 workerCount,
//                                 u64 computeCount,
//                                 u64 nb_scheduler);

// ocrPolicyDomain_t* get_current_policy_domain ();

// ocrGuid_t policy_domain_handIn_assert ( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid );
// ocrGuid_t policy_domain_extract_assert ( ocrPolicyDomain_t * this, ocrPolicyDomain_t * takingPolicy, ocrGuid_t takingWorkerGuid );

// void policy_domain_handOut_assert ( ocrPolicyDomain_t * thisPolicy, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid );
// void policy_domain_receive_assert ( ocrPolicyDomain_t * thisPolicy, ocrGuid_t giverWorkerGuid, ocrGuid_t givenTaskGuid );

#endif /* OCR_POLICY_DOMAIN_H_ */
