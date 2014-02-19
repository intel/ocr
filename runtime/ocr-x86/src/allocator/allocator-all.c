/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "allocator/allocator-all.h"
#include "debug.h"

const char * allocator_types[] = {
    "tlsf",
    "null",
    NULL
};

ocrAllocatorFactory_t *newAllocatorFactory(allocatorType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_ALLOCATOR_TLSF
    case allocatorTlsf_id:
        return newAllocatorFactoryTlsf(typeArg);
#endif
#ifdef ENABLE_ALLOCATOR_NULL
    case allocatorNull_id:
        return newAllocatorFactoryNull(typeArg);
#endif
    case allocatorMax_id:
    default:
        ASSERT(0);
        return NULL;
    };
}

void initializeAllocatorOcr(ocrAllocatorFactory_t * factory, ocrAllocator_t * self, ocrParamList_t *perInstance) {
    self->fguid.guid = UNINITIALIZED_GUID;
    self->fguid.metaDataPtr = self;
    self->pd = NULL;

    self->fcts = factory->allocFcts;
    self->memories = NULL;
    self->memoryCount = 0;
}
