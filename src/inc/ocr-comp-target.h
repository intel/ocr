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

#ifndef __OCR_COMP_TARGET_H__
#define __OCR_COMP_TARGET_H__

#include "ocr-guid.h"
#include "ocr-runtime-def.h"
#include "ocr-worker.h"
#include "ocr-comp-platform.h"


/******************************************************/
/* OCR COMP TARGET FACTORY                            */
/******************************************************/

// Forward declaration
struct ocrCompTarget_t;

typedef struct ocrCompTargetFactory_t {
    struct ocrCompTarget_t * (*instantiate) ( struct ocrCompTargetFactory_t * factory, void * per_type_configuration, void * per_instance_configuration);
    void (*destruct)(struct ocrCompTargetFactory_t * factory);
} ocrCompTargetFactory_t;


/******************************************************/
/* OCR COMP TARGET INTERFACE                          */
/******************************************************/

/*! \brief Abstract class to represent OCR comp-target
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  Currently, we allow comp-target to be started and stopped
 */
typedef struct ocrCompTarget_t {
    ocr_module_t module;
    ocr_comp_platform_t * platform;

    void (*destruct) (struct ocrCompTarget_t * base);

    /*! \brief Starts a thread of execution with a function pointer and and argument for a given stack size
     *
     *  The signature of the interface restricts the routine that can be assigned to a thread as follows.
     *  The function, routine, should take a void pointer, arg, as an argument and return a void pointer
     */
    void (*start) (struct ocrCompTarget_t * base);

    /*! \brief Stops this comp-target
     */
    void (*stop) (struct ocrCompTarget_t * base);

} ocrCompTarget_t;

ocrGuid_t ocr_get_current_worker_guid();


/******************************************************/
/* OCR COMP TARGET KINDS AND CONSTRUCTORS             */
/******************************************************/

typedef enum ocr_comp_target_kind_enum {
    OCR_COMP_TARGET_HC = 1,
    OCR_COMP_TARGET_XE = 2,
    OCR_COMP_TARGET_CE = 3
} ocr_comp_target_kind;

ocrCompTarget_t * newCompTarget(ocr_comp_target_kind compTargetType, void * per_type_configuration, void * per_instance_configuration);

#endif /* __OCR_COMP_TARGET_H__ */
