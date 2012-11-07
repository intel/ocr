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

#ifndef __OCR_FUNCTION_ID_H__
#define __OCR_FUNCTION_ID_H__

#include "ocr-guid.h"
#include "ocr-types.h"
#include "ocr-utils.h"
#include "ocr-edt.h"


/* The structure to hold a function pointer in the hashtable */
struct ocr_hashtable_entry_struct;
typedef struct ocr_hashtable_entry_struct{
  ocrEdt_t func; /* the function pointer */
  u32 id;
  struct ocr_hashtable_entry_struct * nxt; /* the next bucket in the hashtable */
}ocr_hashtable_entry;



/**
 * @brief initializing the data structures to store the function ID mapping on each distributed node
 *
 * @param numFuncs   The size of the function pointer array
 * @param funcs      The array containing the pointers to all the functions in the program that are used as EDTs
 *
 */
void create_function_IDs(u32 numFuncs, ocrEdt_t * funcs);


#endif /* __OCR_FUNCTION_ID_H__ */
