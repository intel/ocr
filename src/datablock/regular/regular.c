/**
 * @brief Simple implementation of a malloc wrapper
 * @authors Romain Cledat, Intel Corporation
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include <stdlib.h>
#include "datablock/regular/regular.h"

void regularCreate(ocrDataBlock_t *self, ocrAllocator_t* allocator, void* address, u64 size,
                   u16 flags, void* configuration) {

    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    rself->base.guid = guidify(self);
    rself->ptr = address;
    rself->allocator = allocator;
    rself->size = size;
    rself->attributess.flags = flags;
    rself->attribute.numUsers = 0;
    rself->attributes.freeRequested = 0;
    ocrGuidTrackerInit(&(rself->usersTracker));
}

void regularDestruct(ocrDataBlock_t *self) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;
    ASSERT(rself->attributes.numUsers == 0);
    ASSERT(rself->attributes.freeRequested == 0);
    // TODO: Inform destruction of GUID
}

void* regularAcquire(ocrDataBlock_t *self, ocrGuid_t edt) {
    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    // Critical section
    rself->lock->lock(rself->lock);
    if(rself->attributes.freeRequested) {
        rself->lock->unlock(rself->lock);
        return NULL;
    }
    u32 idForEdt = ocrGuidTrackerTrack(&(rself->usersTracker), edt, 0);
    if(idForEdt > 63) {
        rself->lock->unlock(rself->lock);
        return NULL;
    }
    rself->attributes.numUsers += 1;
    rself->lock->unlock(rself->lock);
    // End critical section

    // This should be fine as quantity written is full 64 bits
    rself->usersTracker.slots[idForEdt].id =
        ((ocr_task_t*)deguidify(edt))->add_acquired((ocr_task_t*)deguidify(edt),
                                                    rself->base.guid, idForEdt);

    return rself->ptr;
}

u8 regularRelease(ocrDataBlock_t *self, ocrGuid_t edt,
                  u64 edtId) {

    ocrDataBlockRegular_t *rself = (ocrDataBlockRegular_t*)self;

    // TODO: don't forget special case if edt is NULL_GUID
    // Call remove_acquired if not NULL_GUID
    return 0;
}

ocrDataBlock_t* newDataBlockRegular() {
    ocrDataBlockRegular_t *result = (ocrDataBlockRegular_t*)malloc(sizeof(ocrDataBlockRegular_t));
    result->base.create = &regularCreate;
    result->base.destruct = &regularDestruct;
    result->base.allocate = &regularAcquire;
    result->base.release = &regularRelease;
    result->base.free = &regularFree;

    return (ocrLowMemory_t*)result;
}
