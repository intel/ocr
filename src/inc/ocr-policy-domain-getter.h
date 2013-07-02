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

#ifndef OCR_POLICY_DOMAIN_GETTER_H_
#define OCR_POLICY_DOMAIN_GETTER_H_

struct _ocrPolicyDomain_t;
struct _ocrTaskFactory_t;
struct _ocrEventFactory_t;
struct _ocrDataBlockFactory_t;
struct _ocrLockFactory_t;
struct _ocrGuidProvider_t;
struct _ocrPolicyCtx_t;

// Common functions on all policy domains (ie: not function pointers)
inline struct _ocrTaskTemplateFactory_t*  getTaskTemplateFactoryFromPd(struct _ocrPolicyDomain_t *policy);
inline struct _ocrEventFactory_t* getEventFactoryFromPd(struct _ocrPolicyDomain_t *policy);
inline struct _ocrDataBlockFactory_t* getDataBlockFactoryFromPd(struct _ocrPolicyDomain_t *policy);
inline struct _ocrLockFactory_t* getLockFactoryFromPd(struct _ocrPolicyDomain_t *policy);

// Function pointers determining who is who
extern struct _ocrPolicyCtx_t * (*getCurrentWorkerContext)();
extern void (*setCurrentWorkerContext)(struct _ocrPolicyCtx_t *);

/**
 * @brief Gets the current policy domain for the calling code
 */
extern struct _ocrPolicyDomain_t * (*getCurrentPD)();
extern void (*setCurrentPD)(struct _ocrPolicyDomain_t*);
extern struct _ocrPolicyDomain_t * (*getMasterPD)();


/**
 * @brief Function called to temporarily set the answer of getCurrentPD and
 * getCurrentWorkerContext to something sane when the workers have not
 * yet been brought up
 *
 * This order of boot-up is as follows:
 *     - Bring up the factories needed to build a policy domain
 *     - Bring up a GUID provider
 *     - Build a policy domain (the root one)
 *     - Call setBootPD
 *     - Bring up all other instances and do any mapping needed
 *     - Call setIdentifyingFunctions on the comp platform factory to reset
 *       the getCurrentPD etc properly
 *
 * @param domain    Domain to use as the boot PD
 * @return nothing
 */
void setBootPD(ocrPolicyDomain_t* domain);

#endif /* OCR_POLICY_DOMAIN_GETTER_H_ */
