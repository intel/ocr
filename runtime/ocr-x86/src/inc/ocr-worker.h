/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_WORKER_H__
#define __OCR_WORKER_H__

#include "ocr-comp-target.h"
#include "ocr-runtime-types.h"
#include "ocr-scheduler.h"
#include "ocr-types.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

struct _ocrPolicyDomain_t;
struct _ocrPolicyMsg_t;

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/

typedef struct _paramListWorkerFact_t {
    ocrParamList_t base;
} paramListWorkerFact_t;

typedef struct _paramListWorkerInst_t {
    ocrParamList_t base;
} paramListWorkerInst_t;

/******************************************************/
/* OCR WORKER                                         */
/******************************************************/

struct _ocrWorker_t;
struct _ocrTask_t;

typedef struct _ocrWorkerFcts_t {
    void (*destruct)(struct _ocrWorker_t *self);

    void (*begin)(struct _ocrWorker_t *self, struct _ocrPolicyDomain_t * PD);
    
    /** @brief Start Worker
     */
    void (*start)(struct _ocrWorker_t *self, struct _ocrPolicyDomain_t * PD);

    /** @brief Query the worker to finish its work
     */
    void (*finish)(struct _ocrWorker_t *self);

    /** @brief Stop Worker
     */
    void (*stop)(struct _ocrWorker_t *self);

    /** @brief Check if Worker is still running
     *  @return true if the Worker is running, false otherwise
     */
    bool (*isRunning)(struct _ocrWorker_t *self);

    /**
     * @brief Send a message across to another PD
     *
     * @see ocr-comp-platform.h for more details on this function.
     * The worker will pass to the comp-target which ends up passing
     * to the comp-platform. This function will mostly be a
     * pass-through but included here to allow the PD to be
     * blissfully ignorant of the comp-*
     */
    u8 (*sendMessage)(struct _ocrWorker_t *self, ocrLocation_t target,
                      struct _ocrPolicyMsg_t **message);

    /**
     * @brief Poll for a a message
     *
     * @see ocr-comp-platform.h for more details on this function.
     * The worker will pass to the comp-target which ends up passing
     * to the comp-platform. This function will mostly be a
     * pass-through but included here to allow the PD to be
     * blissfully ignorant of the comp-*
     */
    u8 (*pollMessage)(struct _ocrWorker_t *self, struct _ocrPolicyMsg_t **message);
    
    /**
     * @brief Wait for a message from another PD
     *
     * @see ocr-comp-platform.h for more details on this function.
     * The worker will pass to the comp-target which ends up passing
     * to the comp-platform. This function will mostly be a
     * pass-through but included here to allow the PD to be
     * blissfully ignorant of the comp-*
     */
    u8 (*waitMessage)(struct _ocrWorker_t *self, struct _ocrPolicyMsg_t **message);
} ocrWorkerFcts_t;

typedef struct _ocrWorker_t {
    ocrFatGuid_t fguid;
    struct _ocrPolicyDomain_t *pd;
    ocrLocation_t location;
    ocrWorkerType_t type;
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t *statProcess;
#endif

    ocrCompTarget_t **computes; /**< Compute node(s) associated with this worker */
    u64 computeCount;           /**< Number of compute node(s) associated */

    struct _ocrTask_t *curTask; /**< Currently executing task */
    
    ocrWorkerFcts_t fcts;
} ocrWorker_t;


/****************************************************/
/* OCR WORKER FACTORY                               */
/****************************************************/

typedef struct _ocrWorkerFactory_t {
    ocrWorker_t * (*instantiate) (struct _ocrWorkerFactory_t * factory, ocrLocation_t location,
                                  ocrParamList_t *perInstance);
    void (*destruct)(struct _ocrWorkerFactory_t * factory);
    ocrWorkerFcts_t workerFcts;
} ocrWorkerFactory_t;

#endif /* __OCR_WORKER_H__ */

