/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_POLICY_DOMAIN_GETTER_H_
#define OCR_POLICY_DOMAIN_GETTER_H_

struct _ocrPolicyDomain_t;

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
void setBootPD(struct _ocrPolicyDomain_t* domain);

#endif /* OCR_POLICY_DOMAIN_GETTER_H_ */
