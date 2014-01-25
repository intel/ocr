/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_COMP_PLATFORM_FSIM

#include "debug.h"

#include "ocr-comm-platform.h"
#include "ocr-policy-domain.h"

#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"
#include "ocr-worker.h"

#include "fsim-comp-platform.h"

#define DEBUG_TYPE COMP_PLATFORM

ocrCompPlatform_t* newCompPlatformFsim(ocrCompPlatformFactory_t *factory,
                                       ocrParamList_t *perInstance) {

    ocrCompPlatformFsim_t * compPlatformFsim = (ocrCompPlatformFsim_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformFsim_t), NULL);

    compPlatformFsim->base.comm = NULL;
        
    paramListCompPlatformFsim_t * params =
        (paramListCompPlatformFsim_t *) perInstance;
    
    compPlatformFsim->base.fcts = factory->platformFcts;

    return (ocrCompPlatform_t*)compPlatformFsim;
}

/******************************************************/
/* OCR COMP PLATFORM FSIM FACTORY                  */
/******************************************************/

void destructCompPlatformFactoryFsim(ocrCompPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

void getCurrentEnv(ocrPolicyDomain_t** pd, ocrWorker_t** worker,
                   ocrTask_t **task, ocrPolicyMsg_t* msg) {

}

ocrCompPlatformFactory_t *newCompPlatformFactoryFsim(ocrParamList_t *perType) {
    ocrCompPlatformFactory_t *base = (ocrCompPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrCompPlatformFactoryFsim_t), (void *)1);

    ocrCompPlatformFactoryFsim_t * derived = (ocrCompPlatformFactoryFsim_t *) base;

    base->instantiate = &newCompPlatformFsim;
    base->destruct = &destructCompPlatformFactoryFsim;
    base->platformFcts.destruct = NULL;
    base->platformFcts.begin = NULL;
    base->platformFcts.start = NULL;
    base->platformFcts.stop = NULL;
    base->platformFcts.finish = NULL;
    base->platformFcts.getThrottle = NULL;
    base->platformFcts.setThrottle = NULL;
    base->platformFcts.sendMessage = NULL;
    base->platformFcts.pollMessage = NULL;
    base->platformFcts.waitMessage = NULL;
    base->platformFcts.setCurrentEnv = NULL;

    paramListCompPlatformFsim_t * params =
      (paramListCompPlatformFsim_t *) perType;

    return base;
}
#endif /* ENABLE_COMP_PLATFORM_FSIM */
