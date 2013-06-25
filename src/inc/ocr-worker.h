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

#ifndef __OCR_WORKER_H__
#define __OCR_WORKER_H__

#include "ocr-guid.h"
#include "ocr-mappable.h"
#include "ocr-scheduler.h"
#include "ocr-comp-target.h"

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

typedef struct _ocrWorkerFcts_t {
    //TODO deal with worker id
    void (*destruct) (struct _ocrWorker_t *self);

    /*! \brief Start Worker
     */
    void (*start) (struct _ocrWorker_t *self);

    /*! \brief Stop Worker
     */
    void (*stop) (struct _ocrWorker_t *self);

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

    ocrCompTarget_t *computes; /**< Compute node(s) associated with this worker */
    u32 computeCount;          /**< Number of compute node(s) associated */

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

// TODO: Check these functions and prune if required

/*! \brief Getter for Worker id member field
 *  \return identifier for this Worker
 */
int get_worker_id (ocrWorker_t * worker);
/*! \brief Getter for worker id member field
 */
ocrGuid_t get_worker_guid(ocrWorker_t * worker);

/*! \brief Getter for the scheduler, where this Worker works on
 *  \return Scheduler member field for Worker, the one this Worker works on
 */
ocrScheduler_t * get_worker_scheduler(ocrWorker_t * worker);

/*! \brief Associate a worker to a comp-platform.
 *  \param[in] worker
 */
void associate_comp_platform_and_worker(ocrWorker_t * worker);

/*! \brief Get current calling context Worker's GUID
 *  \return GUID for the Worker instance that is executing the code context this function is called in
 */
ocrGuid_t ocr_get_current_worker_guid();

/*! \brief Get the currently executing worker and return the edt's guid it is currently executing.
 */
ocrGuid_t getCurrentEDT();

/* TODO sagnak restructure code in a more pleasant manner than this
 * exposing some HC worker implementations to be reused for the FSIM-like implementations */
void hc_worker_create ( ocrWorker_t * base, void * perTypeConfig, void * perInstanceConfig);
void destructWorkerHc ( ocrWorker_t * base );
void hc_start_worker(ocrWorker_t * base);
void hc_stop_worker(ocrWorker_t * base);
bool hc_is_running_worker(ocrWorker_t * base);
ocrGuid_t hc_getCurrentEDT (ocrWorker_t * base);
void hc_setCurrentEDT (ocrWorker_t * base, ocrGuid_t curr_edt_guid);
void hc_ocr_module_map_scheduler_to_worker(void * self_module, ocrMappableKind kind, u64 nb_instances, void ** ptr_instances);

#endif /* __OCR_WORKER_H__ */
