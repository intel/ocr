/**
 * @brief Simple implementation of a malloc wrapper
 * @authors Romain Cledat, Intel Corporation
 * Copyright (c) 2012, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *      * Neither the name of the Georgia Institute of Technology nor the
 *        names of its contributors may be used to endorse or promote products
 *        derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROMAIN CLEDAT OR INTEL CORPORATION BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include <stdlib.h>
#include "low-memory/malloc/malloc.h"

/* Methods used for this allocation method */
void mallocCreate(ocrLowMemory_t *self, void* config) {
    return; /* not much to do here */
}

void mallocDestruct(ocrLowMemory_t *self) {
    return; /* same as for create... not much to do here */
}

void* mallocAllocate(ocrLowMemory_t *self, u64 size) {
    return malloc(size);
}

void mallocFree(ocrLowMemory_t *self, void *addr) {
    free(addr);
}

ocrLowMemory_t* newLowMemoryMalloc() {
    // TODO: This will be replaced by the runtime/GUID meta-data allocator
    // For now, we cheat and use good-old malloc which is kind of counter productive with
    // all the trouble we are going through to *not* use malloc...
    ocrLowMemoryMalloc_t *result = (ocrLowMemoryMalloc_t*)malloc(sizeof(ocrLowMemoryMalloc_t));
    result->base.create = &mallocCreate;
    result->base.destruct = &mallocDestruct;
    result->base.allocate = &mallocAllocate;
    result->base.free = &mallocFree;

    return (ocrLowMemory_t*)result;
}
