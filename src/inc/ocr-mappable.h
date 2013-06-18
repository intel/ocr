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

#ifndef OCR_MAPPABLE_H_
#define OCR_MAPPABLE_H_

typedef enum _ocrMappableKind {
    OCR_WORKER = 0,
    OCR_COMP_TARGET = 1,
    OCR_COMP_PLATFORM = 2,
    OCR_WORKPILE = 3,
    OCR_SCHEDULER = 4,
    OCR_ALLOCATOR = 5,
    OCR_MEMORY = 6,
    OCR_POLICY = 7,
    OCR_EDT = 8,
    OCR_EVENT = 9
} ocrMappableKind;


typedef enum _ocrMappingKind {
    ONE_TO_ONE_MAPPING  = 0,
    MANY_TO_ONE_MAPPING = 1,
    ONE_TO_MANY_MAPPING = 2
} ocrMappingKind;

typedef struct _ocrModuleMapping_t {
    ocrMappingKind kind;
    ocrMappableKind from;
    ocrMappableKind to;
} ocrModuleMapping_t;

typedef struct _ocrMappable_t ocrMappable_t;

typedef void (*ocrMapFct_t) (ocrMappable_t * self, ocrMappableKind kind,
                             u64 instanceCount, ocrMappable_t ** instances);

typedef struct _ocrMappable_t {
    ocrMapFct_t mapFct;
} ocrMappable_t;


#endif /* OCR_MAPPABLE_H_ */
