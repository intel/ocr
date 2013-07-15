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

#ifndef __HC_EVENT_H__
#define __HC_EVENT_H__

#include "hc/hc.h"
#include "ocr-event.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef struct {
    ocrEventFactory_t base_factory;
    ocrEventFcts_t finishLatchFcts;
} ocrEventFactoryHc_t;

typedef struct ocrEventHc_t {
    ocrEvent_t base;
    ocrEventTypes_t kind;
} ocrEventHc_t;

typedef struct ocrEventHcAwaitable_t {
    ocrEventHc_t base;
    volatile regNode_t * waiters;
    volatile regNode_t * signalers;
    ocrGuid_t data;
} ocrEventHcAwaitable_t;

typedef struct ocrEventHcSingle_t {
    ocrEventHcAwaitable_t base;
} ocrEventHcSingle_t;

typedef struct ocrEventHcLatch_t {
    ocrEventHcAwaitable_t base;
    volatile int counter;
} ocrEventHcLatch_t;

typedef struct ocrEventHcFinishLatch_t {
    ocrEventHc_t base;
    // Dependences to be signaled
    regNode_t outputEventWaiter;
    regNode_t parentLatchWaiter; // Parent latch when nesting finish scope
    ocrGuid_t ownerGuid; // finish-edt starting the finish scope
    volatile ocrGuid_t returnGuid;
    volatile int counter;
} ocrEventHcFinishLatch_t;

ocrEventFactory_t* newEventFactoryHc(ocrParamList_t *perType);

#endif /* __HC_EVENT_H__ */
