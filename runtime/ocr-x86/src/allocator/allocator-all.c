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

#define DEBUG_TYPE ALLOCATOR

const char * allocator_types[] = {
    "tlsf",
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY
    "mallocproxy",
#endif
    "null",
    NULL
};

ocrAllocatorFactory_t *newAllocatorFactory(allocatorType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_ALLOCATOR_TLSF
    case allocatorTlsf_id:
        return newAllocatorFactoryTlsf(typeArg);
#endif
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY
    case allocatorMallocProxy_id:
        return newAllocatorFactoryMallocProxy(typeArg);
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
    u8 * pPoolHeaderDescr = ((u8 *)(((u64) blockPayloadAddr)-sizeof(u64)));
    DPRINTF(DEBUG_LVL_INFO, "allocatorFreeFunction:  PoolHeaderDescr at 0x%lx is 0x%x\n",
        (u64) pPoolHeaderDescr, *pPoolHeaderDescr);
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_DEFINED((u8) pPoolHeaderDescr, sizeof(u8));
#endif
    u8 poolHeaderDescr;
    GET8(poolHeaderDescr, (u64) pPoolHeaderDescr);
#ifdef ENABLE_VALGRIND
    VALGRIND_MAKE_MEM_NOACCESS((u8) pPoolHeaderDescr, sizeof(u8));
#endif
    u8 type = poolHeaderDescr & POOL_HEADER_TYPE_MASK;
    switch(type) {
#ifdef ENABLE_ALLOCATOR_TLSF
    case allocatorTlsf_id:
        tlsfDeallocate(blockPayloadAddr);
        return;
#endif
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY
    case allocatorMallocProxy_id:
        mallocProxyDeallocate(blockPayloadAddr);
        return;
#endif
    case allocatorMax_id:
    default:
        ASSERT(0);
        return;
    };
}
