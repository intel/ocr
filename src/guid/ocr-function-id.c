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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ocr-function-id.h"
#include "ocr-macros.h"

/**
 * Global variables to store the data for mapping function IDs to function pointers and vice-versa
 *
 */
u32 nFuncs;
ocrEdt_t * id_to_fn;
ocr_hashtable_entry ** hashtable = NULL;

/**
 * Quick and dirty hash function
 */
u32 ocr_function_hash(ocrEdt_t pointer){
  return (((unsigned long) pointer) >> 6 ) % nFuncs;
}

ocrEdt_t id_to_func(u32 id){
  assert(id < nFuncs);
  return id_to_fn[id];
}


ocr_hashtable_entry* find_func_entry(ocrEdt_t func){
  u32 bucket;
  bucket = ocr_function_hash(func);
  ocr_hashtable_entry * current = hashtable[bucket];
  while(current != NULL){
    if (current->func == func) return current;
    current = current->nxt;
  }
  return NULL;
}

u32 func_to_id(ocrEdt_t func){
  ocr_hashtable_entry * entry = find_func_entry(func);
  if(entry == NULL){
    printf("Fatal failure: Did not find an ID for the function that was used for an EDT creation. Make sure ALL such functions are passed to the OCR_INIT\n");
    assert(0);
    return 0;
  }
  return entry->id;
}


/**
 * @brief initializing the data structures to store the function ID mapping on each distributed node
 *
 * @param numFuncs   The size of the function pointer array
 * @param funcs      The array containing the pointers to all the functions in the program that are used as EDTs
 *
 */
void create_function_IDs(u32 numFuncs, ocrEdt_t * funcs) {
  nFuncs = numFuncs;
  id_to_fn = funcs;
  //TODO: implement the creation of the hashmap from function pointers to the function ID's
  hashtable = (ocr_hashtable_entry **) malloc(nFuncs*sizeof(ocr_hashtable_entry*));
  assert(hashtable!=NULL);
  u32 i;
  for (i=0; i < numFuncs; i++) hashtable[i] = NULL;
  for (i=0; i < numFuncs; i++){
    ocrEdt_t func = funcs[i];
    if(find_func_entry(func) != NULL){
      printf("Fatal error: Multiple addition of the same function pointer to the OCR_INIT. Make sure you add each function pointer only once!\n");
      assert(0);
    }
    u32 bucket;
    bucket = ocr_function_hash(func);
    ocr_hashtable_entry *newEntry = checked_malloc(newEntry, sizeof(ocr_hashtable_entry));
    assert(newEntry != NULL);
    newEntry->nxt = hashtable[bucket];
    newEntry->func = func;
    newEntry->id = i;
    hashtable[bucket] = newEntry;
    //printf("Creating a function ID for function %lu, with an ID %d, mapping onto a bucket %d\n", (unsigned long) func, (int) i, (int)bucket);
    //printf("Now testing the hashtable. The obtained ID of function %lu is %d\n", (unsigned long) func, (int) func_to_id(func));
  }
}
