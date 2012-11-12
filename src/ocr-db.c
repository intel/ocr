/**
 * @brief Data-block implementation for OCR
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-11-09
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
 */

#include "ocr-db.h"
#include "ocr-datablock.h"
#include "ocr-allocator.h"
#include "ocr-low-workers.h"
#include "ocr-policy.h"
#include "debug.h"
#include <errno.h>

u8 ocrDbCreate(ocrGuid_t *db, void** addr, u64 len, u16 flags,
               ocrLocation_t *location, ocrInDbAllocator_t allocator) {

    // TODO: Currently location and allocator are ignored
    ocrDataBlock_t *createdDb = newDataBlock(OCR_DATABLOCK_DEFAULT);

    // TODO: I need to get the current policy to figure out my allocator.
    // Replace with allocator that is gotten from policy

    ocrGuid_t workerGuid = ocr_get_current_worker_guid();
    ocr_worker_t *worker = (ocr_worker_t*)deguidify(workerGuid);

    ocr_policy_domain_t *policy = (ocr_policy_domain_t*)deguidify(worker->getCurrentPolicyDomain(worker));

    createdDb->create(createdDb, policy->getAllocator(policy, location), len, flags, NULL);

    *addr = createdDb->acquire(createdDb, worker->getCurrentEDT(worker), false);
    if(*addr == NULL) return ENOMEM;
    *db = guidify(createdDb);
    return 0;
}

u8 ocrDbDestroy(ocrGuid_t db) {
    ocrDataBlock_t *dataBlock = (ocrDataBlock_t*)deguidify(db);

    ocrGuid_t workerGuid = ocr_get_current_worker_guid();
    ocr_worker_t *worker = (ocr_worker_t*)deguidify(workerGuid);
    u8 status = dataBlock->free(dataBlock, worker->getCurrentEDT(worker));
    return status;
}

u8 ocrDbAcquire(ocrGuid_t db, void** addr, u16 flags) {
    ocrDataBlock_t *dataBlock = (ocrDataBlock_t*)deguidify(db);

    ocrGuid_t workerGuid = ocr_get_current_worker_guid();
    ocr_worker_t *worker = (ocr_worker_t*)deguidify(workerGuid);
    *addr = dataBlock->acquire(dataBlock, worker->getCurrentEDT(worker), false);
    if(*addr == NULL) return EPERM;
    return 0;
}

u8 ocrDbRelease(ocrGuid_t db) {
    ocrDataBlock_t *dataBlock = (ocrDataBlock_t*)deguidify(db);

    ocrGuid_t workerGuid = ocr_get_current_worker_guid();
    ocr_worker_t *worker = (ocr_worker_t*)deguidify(workerGuid);

    return dataBlock->release(dataBlock, worker->getCurrentEDT(worker), false);
}

u8 ocrDbMalloc(ocrGuid_t guid, u64 size, void** addr) {
    return EINVAL; /* not yet implemented */
}

u8 ocrDbMallocOffset(ocrGuid_t guid, u64 size, u64* offset) {
    return EINVAL; /* not yet implemented */
}

u8 ocrDbFree(ocrGuid_t guid, void* addr) {
    return EINVAL; /* not yet implemented */
}

u8 ocrDbFreeOffset(ocrGuid_t guid, u64 offset) {
    return EINVAL; /* not yet implemented */
}
