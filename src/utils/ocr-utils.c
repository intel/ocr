/**
 * @brief Utility functions for OCR
 * @authors Romain Cledat, Intel Corporation
 *
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

#include "ocr-utils.h"
#include "ocr-types.h"
#include "debug.h"

u32 fls16(u16 val) {
    u32 bit = 15;

    if(!(val & 0xff00)) { val <<= 8; bit -= 8; }
    if(!(val & 0xf000)) { val <<= 4; bit -= 4; }
    if(!(val & 0xc000)) { val <<= 2; bit -= 2; }
    if(!(val & 0x8000)) { val <<= 1; bit -= 1; }

    return bit;
}

u32 fls32(u32 val) {
    u32 bit = 31;

    if(!(val & 0xffff0000)) { val <<= 16; bit -= 16; }
    if(!(val & 0xff000000)) { val <<= 8; bit -= 8; }
    if(!(val & 0xf0000000)) { val <<= 4; bit -= 4; }
    if(!(val & 0xc0000000)) { val <<= 2; bit -= 2; }
    if(!(val & 0x80000000)) { val <<= 1; bit -= 1; }

    return bit;
}

u32 fls64(u64 val) {
    u32 bit = 63;

    if(!(val & 0xffffffff00000000)) { val <<= 32; bit -= 32; }
    if(!(val & 0xffff000000000000)) { val <<= 16; bit -= 16; }
    if(!(val & 0xff00000000000000)) { val <<= 8; bit -= 8; }
    if(!(val & 0xf000000000000000)) { val <<= 4; bit -= 4; }
    if(!(val & 0xc000000000000000)) { val <<= 2; bit -= 2; }
    if(!(val & 0x8000000000000000)) { val <<= 1; bit -= 1; }

    return bit;
}

void ocrGuidTrackerInit(ocrGuidTracker_t *self) {
    self->slotsStatus = 0xFFFFFFFFFFFFFFFFULL;
}

u32 ocrGuidTrackerTrack(ocrGuidTracker_t *self, ocrGuid_t toTrack) {
    u32 slot = 64;
    if(self->slotsStatus == 0) return slot;
    slot = fls64(self->slotsStatus);
    self->slotsStatus &= ~(1ULL<<slot);
    ASSERT(slot <= 63);
    self->slots[slot] = toTrack;
    return slot;
}

bool ocrGuidTrackerRemove(ocrGuidTracker_t *self, ocrGuid_t toTrack, u32 id) {
    if(id > 63) return false;
    if(self->slots[id] != toTrack) return false;

    self->slotsStatus |= (1ULL<<(id));
    return true;
}

u32 ocrGuidTrackerIterateAndClear(ocrGuidTracker_t *self) {
    u64 rstatus = ~(self->slotsStatus);
    u32 slot;
    if(rstatus) return 64;
    slot = fls64(rstatus);
    self->slotsStatus |= (1ULL << slot);
    return slot;
}

u32 ocrGuidTrackerFind(ocrGuidTracker_t *self, ocrGuid_t toFind) {
    u32 result = 64, slot;
    u64 rstatus = ~(self->slotsStatus);
    while(rstatus) {
        slot = fls64(rstatus);
        rstatus &= ~(1ULL << slot);
        slot = slot;
        if(self->slots[slot] == toFind) {
            result = slot;
            break;
        }
    }
    return result;
}
