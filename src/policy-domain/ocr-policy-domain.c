/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"

static ocrPolicyDomain_t* bootPD = NULL;
static ocrPolicyCtx_t* bootCtx = NULL;

static ocrPolicyCtx_t* bootGetCurrentWorkerContext() {
    ASSERT(bootCtx != NULL);
    return bootCtx;
}

static ocrPolicyDomain_t* bootGetCurrentPD() {
    ASSERT(bootPD != NULL);
    return bootPD;

}

static ocrPolicyDomain_t* bootGetMasterPD() {
    ASSERT(bootPD != NULL);
    return bootPD;
}

ocrPolicyCtx_t* (*getCurrentWorkerContext)() = NULL;
void (*setCurrentWorkerContext)(ocrPolicyCtx_t*) = NULL;
ocrPolicyDomain_t* (*getCurrentPD)() = NULL;
void (*setCurrentPD)(ocrPolicyDomain_t*) = NULL;
ocrPolicyDomain_t * (*getMasterPD)() = NULL;

void setBootPD(ocrPolicyDomain_t *domain) {

    bootPD = domain;
    bootCtx = bootPD->getContext(bootPD);
    getCurrentWorkerContext = bootGetCurrentWorkerContext;
    getCurrentPD = bootGetCurrentPD;
    getMasterPD = bootGetMasterPD;
}
