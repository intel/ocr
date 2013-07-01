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

#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-task.h"
#include "ocr-event.h"

inline ocrTaskTemplateFactory_t* getTaskTemplateFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->taskTemplateFactory;
}

inline ocrEventFactory_t* getEventFactoryFromPd(ocrPolicyDomain_t *policy) {
 return policy->eventFactory;   
}

inline ocrDataBlockFactory_t* getDataBlockFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->dbFactory;
}

inline ocrLockFactory_t* getLockFactoryFromPd(ocrPolicyDomain_t *policy) {
  return policy->lockFactory;
}

struct _ocrPolicyDomain_t * (*getMasterPD)() = NULL;

