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

#ifndef HC_H_
#define HC_H_

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include "ocr-runtime.h"
#include "deque.h"
#include "hc_edf.h"


/******************************************************/
/* OCR-HC COMP TARGET                                 */
/******************************************************/

typedef struct {
  ocrCompTargetFactory_t base;
} ocrCompTargetFactoryHc_t;


/******************************************************/
/* OCR-HC WORKER                                      */
/******************************************************/

typedef struct {
  ocrWorkerFactory_t base;
} ocrWorkerFactoryHc_t;

typedef struct {
    ocrWorker_t worker;
    //TODO this is a convenience to map workers to workpiles
    int id;
    //TODO shall these stay here or go up ?
    bool run;
    // reference to the EDT this worker is currently executing
    ocrGuid_t currentEDT_guid;
} ocrWorkerHc_t;


/******************************************************/
/* OCR-HC Workpile                                    */
/******************************************************/

typedef struct {
  ocrWorkpileFactory_t base;
} ocrWorkpileFactoryHc_t;

typedef struct {
    ocrWorkpile_t base;
    deque_t * deque;
} ocrWorkpileHc_t;


/******************************************************/
/* OCR-HC SCHEDULER                                   */
/******************************************************/

typedef struct {
  ocrSchedulerFactory_t base;
} ocrSchedulerFactoryHc_t;

typedef struct {
    ocrScheduler_t scheduler;
    u64 n_pools;
    ocrWorkpile_t ** pools;
    // Note: cache steal iterators in hc's scheduler
    // Each worker has its own steal iterator instantiated
    // a sheduler's construction time.
    ocrWorkpileIterator_t ** steal_iterators;

    int n_workers_per_scheduler;
    u64 worker_id_begin;
    u64 worker_id_end;
} ocrSchedulerHc_t;


/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

typedef struct {
    ocrTaskFactory_t base_factory;
} ocrTaskFactoryHc_t;

ocrTaskFactory_t * newTaskFactoryHc(void * config);

/*
 * TODO HC implementation exposed to support FSIM
 */
extern void hcTaskConstructInternal (ocrTaskHc_t* derived, ocrEdt_t funcPtr,
        u32 paramc, u64 * params, void** paramv, u64 nbDeps, ocrGuid_t outputEvent, ocrTaskFcts_t * taskFctPtrs);

extern void taskSchedule( ocrGuid_t guid, ocrTask_t* base, ocrGuid_t wid );

extern void tryScheduleTask( ocrTask_t* base, ocrGuid_t wid );

#endif /* HC_H_ */
