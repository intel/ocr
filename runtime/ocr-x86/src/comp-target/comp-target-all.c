/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "comp-target/comp-target-all.h"
#include "debug.h"

const char * comptarget_types[] = {
    "PASSTHROUGH",
    NULL
};

ocrCompTargetFactory_t *newCompTargetFactory(compTargetType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_COMP_TARGET_PASSTHROUGH
    case compTargetPassThrough_id:
        return newCompTargetFactoryPt(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

void initializeCompTargetOcr(ocrCompTargetFactory_t * factory, ocrCompTarget_t * self, ocrParamList_t *perInstance) {
    self->fguid.guid = UNINITIALIZED_GUID;
    self->fguid.metaDataPtr = self;

    self->fcts = factory->targetFcts;
    self->platforms = NULL;
    self->platformCount = 0;
}
