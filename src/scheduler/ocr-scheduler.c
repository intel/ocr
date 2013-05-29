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
#include <stdlib.h>

#include "ocr-scheduler.h"

extern ocrScheduler_t * newSchedulerHc(void * per_type_configuration, void * per_instance_configuration);
extern ocrScheduler_t * newSchedulerHcPlaced(void * per_type_configuration, void * per_instance_configuration);
extern ocrScheduler_t * newSchedulerFsimXE(void * per_type_configuration, void * per_instance_configuration);
extern ocrScheduler_t * newSchedulerFsimCE(void * per_type_configuration, void * per_instance_configuration);

ocrScheduler_t * newScheduler(ocr_scheduler_kind schedulerType, void * per_type_configuration, void * per_instance_configuration) {
    switch(schedulerType) {
    case OCR_SCHEDULER_WST:
        return newSchedulerHc(per_type_configuration, per_instance_configuration);
    case OCR_SCHEDULER_XE:
        return newSchedulerFsimXE(per_type_configuration, per_instance_configuration);
    case OCR_SCHEDULER_CE:
        return newSchedulerFsimCE(per_type_configuration, per_instance_configuration);
    case OCR_PLACED_SCHEDULER:
        return newSchedulerHcPlaced(per_type_configuration, per_instance_configuration);
    default:
        assert(false && "Unrecognized scheduler kind");
        break;
    }

    return NULL;
}
