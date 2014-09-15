/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "debug.h"
#include "ocr-policy-domain.h"

#include "comm-platform/comm-platform-all.h"

const char * commplatform_types[] = {
    "None",
    "CE",
    "XE",
    "CePthread",
    "XePthread",
    "MPI",
    "GASNet",
    NULL
};

#define DEBUG_TYPE COMM_PLATFORM

ocrCommPlatformFactory_t *newCommPlatformFactory(commPlatformType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_COMM_PLATFORM_NULL
    case commPlatformNull_id:
        return newCommPlatformFactoryNull(typeArg);
#endif
#ifdef ENABLE_COMM_PLATFORM_CE
    case commPlatformCe_id:
        return newCommPlatformFactoryCe(typeArg);
#endif
#ifdef ENABLE_COMM_PLATFORM_XE
    case commPlatformXe_id:
        return newCommPlatformFactoryXe(typeArg);
#endif
#ifdef ENABLE_COMM_PLATFORM_CE_PTHREAD
    case commPlatformCePthread_id:
        return newCommPlatformFactoryCePthread(typeArg);
#endif
#ifdef ENABLE_COMM_PLATFORM_XE_PTHREAD
    case commPlatformXePthread_id:
        return newCommPlatformFactoryXePthread(typeArg);
#endif
#ifdef ENABLE_COMM_PLATFORM_MPI
    case commPlatformMPI_id:
        return newCommPlatformFactoryMPI(typeArg);
#endif
#ifdef ENABLE_COMM_PLATFORM_GASNET
    case commPlatformGasnet_id:
        return newCommPlatformFactoryGasnet(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

void initializeCommPlatformOcr(ocrCommPlatformFactory_t * factory, ocrCommPlatform_t * self, ocrParamList_t *perInstance) {
    self->location = ((paramListCommPlatformInst_t *)perInstance)->location;
    self->fcts = factory->platformFcts;
}
