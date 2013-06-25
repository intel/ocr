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

#ifndef __WORKPILE_ALL_H__
#define __WORKPILE_ALL_H__

#include "ocr-workpile.h"
#include "ocr-utils.h"

typedef enum _workpileType_t {
    workpileHc_id,
    workpileFsimMessage_id
} workpileType_t;

extern ocrWorkpileFactory_t* newOcrWorkpileFactoryHc(ocrParamList_t *perType);
extern ocrWorkpileFactory_t* newOcrWorkpileFactoryFsimMessage(ocrParamList_t *perType);

inline ocrWorkpileFactory_t * newWorkpileFactory(workpileType_t type, ocrParamList_t *perType) {
    switch(type) {
    case workpileHc_id:
        return newOcrWorkpileFactoryHc(perType);
    case workpileFsimMessage_id:
        return newOcrWorkpileFactoryFsimMessage(perType);
    }
    ASSERT(0);
    return NULL;
}

#endif /* __WORKPILE_ALL_H__ */