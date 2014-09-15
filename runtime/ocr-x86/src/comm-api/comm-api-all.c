/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "comm-api/comm-api-all.h"
#include "debug.h"

const char * commapi_types[] = {
    "Delegate",
    "Handleless",
    "Simple",
    NULL
};

ocrCommApiFactory_t *newCommApiFactory(commApiType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_COMM_API_DELEGATE
    case commApiDelegate_id:
        return newCommApiFactoryDelegate(typeArg);
#endif
#ifdef ENABLE_COMM_API_HANDLELESS
    case commApiHandleless_id:
        return newCommApiFactoryHandleless(typeArg);
#endif
#ifdef ENABLE_COMM_API_SIMPLE
    case commApiSimple_id:
        return newCommApiFactorySimple(typeArg);
#endif
    default:
        ASSERT(0);
        return NULL;
    };
}

void initializeCommApiOcr(ocrCommApiFactory_t * factory, ocrCommApi_t * self, ocrParamList_t *perInstance) {
    self->pd = NULL;
    self->commPlatform = NULL;
    self->fcts = factory->apiFcts;
}

