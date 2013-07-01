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

#ifndef __WORKER_ALL_H__
#define __WORKER_ALL_H__

#include "ocr-worker.h"
#include "ocr-utils.h"

typedef enum _workerType_t {
    workerHc_id,
    workerFsimXE_id,
    workerFsimCE_id,
    workerMax_id,
} workerType_t;

const char * worker_types[] = {
    "HC",
    "XE",
    "CE",
    NULL
};

extern ocrWorkerFactory_t* newOcrWorkerFactoryHc(ocrParamList_t *perType);
extern ocrWorkerFactory_t* newOcrWorkerFactoryFsimXE(ocrParamList_t *perType);
extern ocrWorkerFactory_t* newOcrWorkerFactoryFsimCE(ocrParamList_t *perType);

inline ocrWorkerFactory_t * newWorkerFactory(workerType_t type, ocrParamList_t *perType) {
    switch(type) {
    case workerFsimXE_id:
    //DELME    return newOcrWorkerFactoryFsimXE(perType);
    case workerFsimCE_id:
    //DELME    return newOcrWorkerFactoryFsimCE(perType);
    case workerHc_id:
      return newOcrWorkerFactoryHc(perType);
    case workerMax_id:
    default:
      ASSERT(0);
    }
    return NULL;
}

#endif /* __WORKER_ALL_H__ */
