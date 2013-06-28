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

#ifndef __COMP_PLATFORM_ALL_H__
#define __COMP_PLATFORM_ALL_H__

#include "ocr-comp-platform.h"
#include "ocr-utils.h"
#include "debug.h"

typedef enum _compPlatformType_t {
    compPlatformPthread_id,
    compPlatformMax_id,
} compPlatformType_t;

const char * compplatform_types[] = {
    "pthread",
    NULL,
};

// Pthread compute platform
#include "comp-platform/pthread/pthread-comp-platform.h"

// Add other compute platforms using the same pattern as above

inline ocrCompPlatformFactory_t *newCompPlatformFactory(compPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
    case compPlatformPthread_id:
        return newCompPlatformFactoryPthread(typeArg);
    default:
        ASSERT(0);
        return NULL;
    };
}

#endif /* __COMP_PLATFORM_ALL_H__ */
