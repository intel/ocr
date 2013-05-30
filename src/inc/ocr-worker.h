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
#include "ocr-runtime-def.h"
#include "ocr-scheduler.h"


/******************************************************/
/* OCR WORKER                                         */
/******************************************************/

// Forward declaration
struct ocrWorker_t;

typedef struct ocrWorkerFactory_t {
    struct ocrWorker_t * (*instantiate) ( struct ocrWorkerFactory_t * factory, void * per_type_configuration, void * per_instance_configuration);
    void (*destruct)(struct ocrWorkerFactory_t * factory);
} ocrWorkerFactory_t;

typedef struct ocrWorker_t {
    ocr_module_t module;
    ocrGuid_t guid;
    ocrScheduler_t * scheduler;

    /*! \brief Routine the worker executes
     */
    void * (*routine)(void *);

    //TODO deal with worker id
    void (*destruct) (struct ocrWorker_t * base);

    /*! \brief Start Worker
     */
    void (*start) (struct ocrWorker_t * base);

    /*! \brief Stop Worker
     */
    void (*stop) (struct ocrWorker_t * base);

    /*! \brief Check if Worker is still running
     *  \return true if the Worker is running, false otherwise
     */
    bool (*is_running) (struct ocrWorker_t * base);

    /**
     * @brief Returns the EDT this worker is currently running
     *
     * This returns the GUID for the EDT
     * @param base              OCR Worker
     * @return GUID for the currently running EDT
     */
    ocrGuid_t (*getCurrentEDT)(struct ocrWorker_t *base);

    /**
     * @brief Sets the EDT this worker is currently running
     *
     * @param base              OCR Worker
     * @param currEDT           GUID of the EDT this OCR Worker is now running
     * @return GUID for the currently running EDT
     */
     void (*setCurrentEDT)(struct ocrWorker_t *base, ocrGuid_t currEDT);
} ocrWorker_t;

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

/******************************************************/
/* OCR WORKER KINDS AND CONSTRUCTORS                  */
/******************************************************/

typedef enum ocr_worker_kind_enum {
    OCR_WORKER_HC = 1,
    OCR_WORKER_XE = 2,
    OCR_WORKER_CE = 3
} ocr_worker_kind;

ocrWorker_t * newWorker(ocr_worker_kind workerType, void * per_type_configuration, void * per_instance_configuration);

/* we have to end up exposing the configuration declarations too for the runtime model
 * I do not know if abstract factories may help with this situation */
typedef struct worker_configuration {
    size_t worker_id;
} worker_configuration;

/* TODO sagnak restructure code in a more pleasant manner than this
 * exposing some HC worker implementations to be reused for the FSIM-like implementations */
void hc_worker_create ( ocrWorker_t * base, void * per_type_configuration, void * per_instance_configuration);
void destructWorkerHc ( ocrWorker_t * base );
void hc_start_worker(ocrWorker_t * base);
void hc_stop_worker(ocrWorker_t * base);
bool hc_is_running_worker(ocrWorker_t * base);
ocrGuid_t hc_getCurrentEDT (ocrWorker_t * base);
void hc_setCurrentEDT (ocrWorker_t * base, ocrGuid_t curr_edt_guid);
void hc_ocr_module_map_scheduler_to_worker(void * self_module, ocr_module_kind kind, size_t nb_instances, void ** ptr_instances);

#endif /* __OCR_WORKER_H__ */
