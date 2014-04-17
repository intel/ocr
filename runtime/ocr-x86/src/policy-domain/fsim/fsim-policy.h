/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __FSIM_POLICY_H__
#define __FSIM_POLICY_H__

#include "ocr-policy-domain.h"
#include "ocr-utils.h"

typedef struct _ocrPolicyCtxXE_t {
    ocrPolicyCtx_t base;
} ocrPolicyCtxXE_t;

typedef struct _ocrPolicyCtxFactoryXE_t {
    ocrPolicyCtxFactory_t base;
} ocrPolicyCtxFactoryXE_t;

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryXE_t;

typedef struct {
    ocrPolicyDomain_t base;
} ocrPolicyDomainXE_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryXE(ocrParamList_t *perType);
ocrPolicyCtxFactory_t *newPolicyCtxFactoryXE(ocrParamList_t *perType);

typedef struct _ocrPolicyCtxCE_t {
    ocrPolicyCtx_t base;
} ocrPolicyCtxCE_t;

typedef struct _ocrPolicyCtxFactoryCE_t {
    ocrPolicyCtxFactory_t base;
} ocrPolicyCtxFactoryCE_t;

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryCE_t;

typedef struct {
    ocrPolicyDomain_t base;
} ocrPolicyDomainCE_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryCE(ocrParamList_t *perType);
ocrPolicyCtxFactory_t *newPolicyCtxFactoryCE(ocrParamList_t *perType);

typedef struct _ocrPolicyCtxMasteredCE_t {
    ocrPolicyCtx_t base;
} ocrPolicyCtxMasteredCE_t;

typedef struct _ocrPolicyCtxFactoryMasteredCE_t {
    ocrPolicyCtxFactory_t base;
} ocrPolicyCtxFactoryMasteredCE_t;

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryMasteredCE_t;

typedef struct {
    ocrPolicyDomain_t base;
} ocrPolicyDomainMasteredCE_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryMasteredCE(ocrParamList_t *perType);
ocrPolicyCtxFactory_t *newPolicyCtxFactoryMasteredCE(ocrParamList_t *perType);
#endif /* __FSIM_POLICY_H__ */

