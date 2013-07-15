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
#ifndef __HC_TASK_H__
#define __HC_TASK_H__

#include "hc/hc.h"
#include "ocr-task.h"
#include "ocr-utils.h"

/*! \brief Event Driven Task(EDT) template implementation
 */
typedef struct {
    ocrTaskTemplate_t base;
} ocrTaskTemplateHc_t;

/*! \brief Event Driven Task(EDT) implementation for OCR Tasks
 */
typedef struct {
    ocrTask_t base;
    regNode_t * waiters;
    regNode_t * signalers; // Does not grow, set once when the task is created
} ocrTaskHc_t;

typedef struct {
    ocrTaskTemplateFactory_t baseFactory;
} ocrTaskTemplateFactoryHc_t;

ocrTaskTemplateFactory_t * newTaskTemplateFactoryHc(ocrParamList_t* perType);

typedef struct {
    ocrTaskFactory_t baseFactory;
    // singleton the factory passes on to each task instance
    ocrTaskFcts_t taskFctPtrs;
} ocrTaskFactoryHc_t;

ocrTaskFactory_t * newTaskFactoryHc(ocrParamList_t* perType);
#endif /* __HC_TASK_H__ */
