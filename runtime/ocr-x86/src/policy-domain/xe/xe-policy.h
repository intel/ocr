/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __XE_POLICY_H__
#define __XE_POLICY_H__

#include "ocr-config.h"
#ifdef ENABLE_POLICY_DOMAIN_XE

#include "ocr-policy-domain.h"
#include "utils/ocr-utils.h"

/******************************************************/
/* OCR-XE POLICY DOMAIN                               */
/******************************************************/

typedef struct {
    ocrPolicyDomainFactory_t base;
} ocrPolicyDomainFactoryXe_t;

typedef struct {
    ocrPolicyDomain_t base;
    void *packedArgsLocation;  // Keep this here.  If moved around, might make mismatch in
                               // .../ss/rmdkrnl/inc/rmd-bin-file.h, "magic" number XE_PDARGS_OFFSET.
    u8 shutdownInitiated;      // Non-zero if the XE knows about the shutdown
} ocrPolicyDomainXe_t;

ocrPolicyDomainFactory_t *newPolicyDomainFactoryXe(ocrParamList_t *perType);

#ifdef ENABLE_SYSBOOT_FSIM
#include "rmd-bin-files.h"
#define myStaticAssert(e) extern char (*myStaticAssert(void))[sizeof(char[1-2*!(e)])]
myStaticAssert(offsetof(ocrPolicyDomain_t, fcts) + offsetof(ocrPolicyDomainFcts_t, begin) == PDBEGIN_OFFSET); // If this fails, go to ss/common/include/rmd-bin-files.h and change PDBEGIN_OFFSET
myStaticAssert(offsetof(ocrPolicyDomain_t, fcts) + offsetof(ocrPolicyDomainFcts_t, start) == PDSTART_OFFSET); // If this fails, go to ss/common/include/rmd-bin-files.h and change PDSTART_OFFSET
myStaticAssert(offsetof(ocrPolicyDomainXe_t, packedArgsLocation) == XE_PDARGS_OFFSET);                        // If this fails, go to ss/common/include/rmd-bin-files.h and change XE_PDARGS_OFFSET
#endif


#endif /* ENABLE_POLICY_DOMAIN_XE */
#endif /* __XE_POLICY_H__ */
