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

#ifndef __OCR_COMP_PLATFORM_H__
#define __OCR_COMP_PLATFORM_H__

#include "ocr-guid.h"
#include "ocr-runtime-def.h"
#include "ocr-worker.h"


/******************************************************/
/* OCR COMP PLATFORM INTERFACE                        */
/******************************************************/

//Forward declaration
struct ocr_comp_platform_struct;

typedef void * (*comp_platform_routine)(void *);

typedef void (*ocr_comp_platform_create_fct) (struct ocr_comp_platform_struct * base, void * configuration);

typedef void (*ocr_comp_platform_destruct_fct) (struct ocr_comp_platform_struct * base);

/*! \brief Starts a thread of execution with a function pointer and an argument for a given stack size
 *  \param[in]  routine A function that represents the computation this thread runs as it starts
 *  \param[in]  arg Argument to be passed to the routine mentioned above
 *  \param[in]  stack_size Size of stack allowed for this thread invocation
 *
 *  The signature of the interface restricts the routine that can be assigned to a thread as follows.
 *  The function, routine, should take a void pointer, arg, as an argument and return a void pointer
 */
typedef void (*ocr_comp_platform_start_fct) (struct ocr_comp_platform_struct * base);

/*! \brief Stops this thread of execution
 */
typedef void (*ocr_comp_platform_stop_fct) (struct ocr_comp_platform_struct * base);

/*! \brief Abstract class to represent OCR thread of execution.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  Currently, we allow threads to be started and stopped
 */
typedef struct ocr_comp_platform_struct {
    ocrMappable_t module;

    void * (*routine)(void *);
    void * routine_arg;

    ocr_comp_platform_create_fct create;
    ocr_comp_platform_destruct_fct destruct;
    ocr_comp_platform_start_fct start;
    ocr_comp_platform_stop_fct stop;
} ocr_comp_platform_t;

//TODO this is really ocr-hc specific
void associate_comp_platform_and_worker(ocrWorker_t * worker);

ocrGuid_t ocr_get_current_worker_guid();


/******************************************************/
/* OCR COMP PLATFORM KINDS AND CONSTRUCTORS           */
/******************************************************/

typedef enum ocr_comp_platform_kind_enum {
    OCR_COMP_PLATFORM_PTHREAD = 1,
} ocr_comp_platform_kind;

ocr_comp_platform_t * newCompPlatform(ocr_comp_platform_kind compPlatformType);

ocr_comp_platform_t * ocr_comp_platform_pthread_constructor(void);

#endif /* __OCR_COMP_PLATFORM_H__ */
