/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_WORKER_H__
#define __OCR_WORKER_H__

#include "ocr-comp-target.h"
#include "ocr-mappable.h"
#include "ocr-scheduler.h"
#include "ocr-types.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-statistics.h"
#endif

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
    void (*destruct) (struct _ocrWorker_t *self);

    /*! \brief Start Worker
     */
    void (*start) (struct _ocrWorker_t *self, struct _ocrPolicyDomain_t * PD);

    /*! \brief Query the worker to finish its work
     */
    void (*finish) (struct _ocrWorker_t *self);

    /*! \brief Stop Worker
     */
    void (*stop) (struct _ocrWorker_t *self);

    /*! \brief Executes a task
     */
    void (*execute)(struct _ocrWorker_t * worker, struct _ocrTask_t* task, ocrGuid_t taskGuid, ocrGuid_t currentTaskGuid);

    /*! \brief Check if Worker is still running
     *  \return true if the Worker is running, false otherwise
     */
    bool (*isRunning) (struct _ocrWorker_t *self);

    /**
     * @brief Returns the EDT this worker is currently running
     *
     * This returns the GUID for the EDT
     * @param base              OCR Worker
     * @return GUID for the currently running EDT
     */
    ocrGuid_t (*getCurrentEDT)(struct _ocrWorker_t *self);

    /**
     * @brief Sets the EDT this worker is currently running
     *
     * @param base              OCR Worker
     * @param currEDT           GUID of the EDT this OCR Worker is now running
     * @return GUID for the currently running EDT
     */
     void (*setCurrentEDT)(struct _ocrWorker_t *self, ocrGuid_t currEDT);
} ocrWorkerFcts_t;

typedef struct _ocrWorker_t {
    ocrMappable_t module;
    ocrGuid_t guid;
#ifdef OCR_ENABLE_STATISTICS
    ocrStatsProcess_t *statProcess;
#endif

    ocrCompTarget_t **computes; /**< Compute node(s) associated with this worker */
    u64 computeCount;          /**< Number of compute node(s) associated */

    /*! \brief Routine the worker executes
     */
    void * (*routine)(void *);

    ocrWorkerFcts_t *fctPtrs;
} ocrWorker_t;


/****************************************************/
/* OCR WORKER FACTORY                               */
/****************************************************/

typedef struct _ocrWorkerFactory_t {
    ocrMappable_t module;
    ocrWorker_t * (*instantiate) (struct _ocrWorkerFactory_t * factory, ocrParamList_t *perInstance);
    void (*destruct)(struct _ocrWorkerFactory_t * factory);
    ocrWorkerFcts_t workerFcts;
} ocrWorkerFactory_t;


/**
 * @brief Gets the current EDT executing
 */
extern ocrGuid_t (*getCurrentEDT)();
extern void (*setCurrentEDT)(ocrGuid_t guid);

// Get/Set CurrentEDT relying on the worker caching the info
extern ocrGuid_t getCurrentEDTFromWorker();
extern void setCurrentEDTToWorker(ocrGuid_t edtGuid);

#endif /* __OCR_WORKER_H__ */
