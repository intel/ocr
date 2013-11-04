/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "comp-target/hc/hc-comp-target.h"
#include "debug.h"
#include "ocr-comp-platform.h"
#include "ocr-comp-target.h"
#include "ocr-macros.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"

static void hcDestruct(ocrCompTarget_t *compTarget) {
    int i = 0;
    while(i < compTarget->platformCount) {
        compTarget->platforms[i]->fctPtrs->destruct(compTarget->platforms[i]);
        i++;
    }
    free(compTarget->platforms);
#ifdef OCR_ENABLE_STATISTICS
    statsCOMPTARGET_STOP(getCurrentPD(), compTarget->guid, compTarget);
#endif
    free(compTarget);
}

static void hcStart(ocrCompTarget_t * compTarget, ocrPolicyDomain_t * PD, launchArg_t * launchArg) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fctPtrs->start(compTarget->platforms[0], PD, launchArg);
}

static void hcStop(ocrCompTarget_t * compTarget) {
    ASSERT(compTarget->platformCount == 1);
    compTarget->platforms[0]->fctPtrs->stop(compTarget->platforms[0]);
}

ocrCompTarget_t * newCompTargetHc(ocrCompTargetFactory_t * factory, ocrParamList_t* perInstance) {
    ocrCompTargetHc_t * compTarget = checkedMalloc(compTarget, sizeof(ocrCompTargetHc_t));

    // TODO: Setup GUID
    compTarget->base.platforms = NULL;
    compTarget->base.platformCount = 0;
    compTarget->base.fctPtrs = &(factory->targetFcts);

    // TODO: Setup routine and routineArg. Should be in perInstance misc
#ifdef OCR_ENABLE_STATISTICS
    statsCOMPTARGET_START(getCurrentPD(), compTarget->base.guid, &(compTarget->base));
#endif
    
    return (ocrCompTarget_t*)compTarget;
}

/******************************************************/
/* OCR COMP TARGET HC FACTORY                         */
/******************************************************/
static void destructCompTargetFactoryHc(ocrCompTargetFactory_t *factory) {
    free(factory);
}

ocrCompTargetFactory_t *newCompTargetFactoryHc(ocrParamList_t *perType) {
    ocrCompTargetFactory_t *base = (ocrCompTargetFactory_t*)
        checkedMalloc(base, sizeof(ocrCompTargetFactoryHc_t));
    base->instantiate = &newCompTargetHc;
    base->destruct = &destructCompTargetFactoryHc;
    base->targetFcts.destruct = &hcDestruct;
    base->targetFcts.start = &hcStart;
    base->targetFcts.stop = &hcStop;

    return base;
}
