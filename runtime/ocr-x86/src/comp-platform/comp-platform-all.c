/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "comp-platform/comp-platform-all.h"
#include "debug.h"

const char * compplatform_types[] = {
    "pthread",
    "fsim",
    NULL,
};

ocrCompPlatformFactory_t *newCompPlatformFactory(compPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_COMP_PLATFORM_PTHREAD
    case compPlatformPthread_id:
        return newCompPlatformFactoryPthread(typeArg);
#endif
#ifdef ENABLE_COMP_PLATFORM_FSIM
    case compPlatformFsim_id:
        return newCompPlatformFactoryFsim(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

void initializeCompPlatformOcr(ocrCompPlatformFactory_t * factory, ocrCompPlatform_t * self, ocrParamList_t *perInstance) {
    self->fcts = factory->platformFcts;
}
