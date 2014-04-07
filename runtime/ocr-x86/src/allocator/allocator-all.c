/*
* This file is subject to the license agreement located in the file LICENSE
* and cannot be distributed without it. This notice cannot be
* removed or modified.
*/

#include "allocator/allocator-all.h"
#include "debug.h"
#ifdef ENABLE_VALGRIND
#include <valgrind/memcheck.h>
#include "ocr-hal.h"
#endif

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

void allocatorFreeFunction(void* blockPayloadAddr) {
    u64 * pPoolHeaderDescr = ((u64 *)(((u64) blockPayloadAddr)-sizeof(u64)));
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED((u64) pPoolHeaderDescr, sizeof(u64));
#endif
    u64 poolHeaderDescr;
    GET64(poolHeaderDescr, (u64) pPoolHeaderDescr);
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS((u64) pPoolHeaderDescr, sizeof(u64));
#endif
    u64 type = poolHeaderDescr & POOL_HEADER_TYPE_MASK;
    switch(type) {
#ifdef ENABLE_ALLOCATOR_TLSF
    case allocatorTlsf_id:
        tlsfDeallocate(blockPayloadAddr);
        return;
#endif
    case allocatorMax_id:
    default:
        ASSERT(0);
        return;
    };
}
