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

typedef struct _ocrPolicyCtxHC_t {
    ocrPolicyCtx_t base;
} ocrPolicyCtxHC_t;

typedef struct _ocrPolicyCtxFactoryHC_t {
    ocrPolicyCtxFactory_t base;
} ocrPolicyCtxFactoryHC_t;

/******************************************************/
/* OCR-HC POLICY DOMAIN                               */
/******************************************************/

typedef struct {
  ocrPolicyDomainFactory_t base;
} ocrPolicyDomainHcFactory_t;

typedef struct {
  ocrPolicyDomain_t base;
} ocrPolicyDomainHc_t;

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

typedef struct _paramListWorkerHcInst_t {
  paramListWorkerInst_t base;
  int workerId;
} paramListWorkerHcInst_t;

typedef struct {
    ocrWorker_t worker;
    // The HC implementation relies on integer ids to
    // map workers, schedulers and workpiles together
    int id;
    // Flag the worker checksto now if he's running
    bool run;
    // reference to the EDT this worker is currently executing
    ocrGuid_t currentEDTGuid;
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
    // Note: cache steal iterators in hc's scheduler
    // Each worker has its own steal iterator instantiated
    // a sheduler's construction time.
    ocrWorkpileIterator_t ** steal_iterators;
    u64 worker_id_first;
} ocrSchedulerHc_t;

typedef struct _paramListSchedulerHcInst_t {
  paramListSchedulerInst_t base;
  int worker_id_first;
} paramListSchedulerHcInst_t;

/******************************************************/
/* OCR-HC Task Factory                                */
/******************************************************/

typedef struct {
    ocrTaskTemplateFactory_t base_factory;
} ocrTaskTemplateFactoryHc_t;

ocrTaskTemplateFactory_t * newTaskTemplateFactoryHc(ocrParamList_t* perType);

typedef struct {
    ocrTaskFactory_t base_factory;
    // singleton the factory passes on to each task instance
    ocrTaskFcts_t taskFctPtrs;
} ocrTaskFactoryHc_t;

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perType);


//
// Guid-kind checkers for convenience
//

bool isDatablockGuid(ocrGuid_t guid);

bool isEventGuid(ocrGuid_t guid);

bool isEdtGuid(ocrGuid_t guid);

bool isEventLatchGuid(ocrGuid_t guid);

bool isEventSingleGuid(ocrGuid_t guid);

#endif /* HC_H_ */
