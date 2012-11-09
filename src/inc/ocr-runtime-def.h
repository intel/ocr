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

#ifndef OCR_RUNTIME_DEF_H_
#define OCR_RUNTIME_DEF_H_

typedef enum ocr_module_kind_enum {
    OCR_WORKER = 0,
    OCR_EXECUTOR = 1,
    OCR_WORKPILE = 2,
    OCR_SCHEDULER = 3,
    OCR_ALLOCATOR = 4,
    OCR_MEMORY = 5,
    OCR_POLICY = 6
} ocr_module_kind;


typedef enum ocr_module_mapping_kind_enum {
    ONE_TO_ONE_MAPPING = 0,
    MANY_TO_ONE_MAPPING = 1,
    ONE_TO_MANY_MAPPING = 2
} ocr_mapping_kind;

typedef struct struct_ocr_module_mapping_t {
    ocr_mapping_kind kind;
    ocr_module_kind from;
    ocr_module_kind to;
} ocr_module_mapping_t;

typedef void (*ocr_module_map_fct) (void * self_module, ocr_module_kind kind, size_t nb_instances, void ** ptr_instances);

typedef struct struct_ocr_module_t {
    ocr_module_map_fct map_fct;
} ocr_module_t;


#endif /* OCR_RUNTIME_DEF_H_ */
