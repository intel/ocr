/**
 * @brief Implementation of an allocator based on passing through to the system malloc/free.  Not for use on FSIM.
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

// This implements a pass-through to the system malloc/realloc/free functions, for the purposes of
// easing debugging of applications; bugs in an application can be isolated more easily if the
// application programmer is not suspicious of our allocators, so (s)he might have more confidence
// in the system facilities.  They are NOT available for the FSIM platform.
//
// In this implementation, we have made provision for the possibility that the user might wish to
// have some other kinds of allocators, mixed with malloc.  For example, it might be desirable to
// use per-agent L1 TLSF allocators, and then to drop into using system facilities for datablocks
// that cannot be accomodated by those per-agent allocators.  Or perhaps L1, L2, and L3 TLSF, and
// then a single system malloc to handle everything higher.  To allow for this possibility, when
// the datablock is deallocated, it is necessary to be able to disambiguate between those created
// by system malloc versus those created by other kinds of allocators.  For this reason, we have to
// have a "wrapper" around the user's payload, that from the perspective of the system malloc would
// be ITS payload.  That is, we have to have our own datablock header, apart from whatever the
// system malloc might implement for its own record-keeping.  Because we have to put this "wrapper"
// around the user's requested datablock, we call this allocator a "proxy" for the underlying
// familiar system malloc/realloc/free functions.
//
// While we are going about putting in our own datablock header "wrapper", we also go to the effort
// of assuring that the payload that we return to the user will be eight-byte aligned, a service that
// is not otherwise guaranteed by the system malloc.

#include "ocr-config.h"
#ifdef ENABLE_ALLOCATOR_MALLOCPROXY

#include "debug.h"
#include "ocr-sysboot.h"
#include "ocr-types.h"
#include "mallocproxy-allocator.h"
#include "allocator/allocator-all.h"

#define DEBUG_TYPE ALLOCATOR

//#ifndef ENABLE_BUILDER_ONLY

#ifdef OCR_ENABLE_STATISTICS
//#include "ocr-statistics.h"
//#include "ocr-statistics-callbacks.h"
#endif

#ifdef ENABLE_VALGRIND
#include <valgrind/memcheck.h>
#define VALGRIND_DEFINED(addr) VALGRIND_MAKE_MEM_DEFINED((addr), sizeof(blkHdr_t))
#define VALGRIND_NOACCESS(addr) VALGRIND_MAKE_MEM_NOACCESS((addr), sizeof(blkHdr_t))
#else
#define VALGRIND_DEFINED(addr)
#define VALGRIND_NOACCESS(addr)
#endif

/******************************************************/
/* OCR ALLOCATOR MALLOCPROXY IMPLEMENTATION           */
/******************************************************/

#define ALIGNMENT 8LL  // Not fundamentally required, but convenient;  it matches what is done in
                       // TLSF.  It could probably be reduced to 4LL for this code without much
                       // grief (and very little advantage).  Less than that, access to the
                       // checksum in the blkHdr_t would become potentially unaligned.

typedef struct blkHdr_t {            // Header of an allocated block of memory.
    union {
        u32 stuffToChecksum;
        struct {
            u8  allocatorType;       // The placement of this value corresponds to the convention for ALL
                                     // types of allocators.  It is 8 bytes back from the payload address.
                                     // Low 3 bits index to the type of allocator in use.  This MALLOC
                                     // allocator is one of those index values, and other [potential]
                                     // allocators are other values.  This is used to index into the
                                     // appropriate "free" function.
            u8  distToMallocedBlock; // Number of bytes back from payload address returned to user to the
                                     // address returned by the system malloc.
            u16 junk;                // Not used.
        };
    };
    u32 checksum;                    // Checksum, when ENABLE_ALLOCATOR_CHECKSUM is #defined.  Else not used.
} blkHdr_t;

#ifdef ENABLE_ALLOCATOR_CHECKSUM
static inline u32 calcChecksum (blkHdr_t * blkAddr) {
    return (0xFeedF00dL ^ (blkAddr->stuffToChecksum << 5) ^ (blkAddr->stuffToChecksum >> 27));
}

static inline void setChecksum (blkHdr_t * blkAddr) {
    blkAddr->junk = 999;  // Obscure but defined.  This contributes to the checksum.
    blkAddr->checksum = calcChecksum(blkAddr);
}

static inline void checkChecksum (blkHdr_t * blkAddr, u32 linenum) {
    if (calcChecksum(blkAddr) != blkAddr->checksum) {
        DPRINTF (DEBUG_LVL_WARN, "Checksum failure detected at %s line %d\n", __FILE__, linenum);
        ASSERT(0);
    }
}
#else
#define setChecksum(blkAddr)
#define checkChecksum(blkAddr, linenum)
#endif


// If we were C++, the following would be "access methods", to "private" data of various blkHdr_t classes:

// Get/Set number of bytes from payload address we return to user to address system malloc gives to us.
// (Difference is for our wrapper, and wastage to provide alignment.)
static inline u8 GET_distToMallocedBlock (blkHdr_t * pBlk) {
    return pBlk->distToMallocedBlock;
}

static inline void SET_distToMallocedBlock (blkHdr_t * pBlk, u8 value) {
    pBlk->distToMallocedBlock = value;
}

// Get/Set that this is the malloc proxy.  Needed by deallocate function, to know which type of allocator
// is associated with a block being freed.  As such, ALL allocators must agree on how and where to get
// this value.
static inline u8 GET_allocatorType (blkHdr_t * pBlk) {
    return pBlk->allocatorType;
}

static inline void SET_allocatorType (blkHdr_t * pBlk, u8 value) {
    pBlk->allocatorType = value;
}

#define myStaticAssert(e) extern char (*myStaticAssert(void))[sizeof(char[1-2*!(e)])]

myStaticAssert(ALIGNMENT == 8LL);
myStaticAssert((ALIGNMENT-1) == POOL_HEADER_TYPE_MASK);

// Some assertions to make sure things are OK
myStaticAssert(sizeof(blkHdr_t) == 8);
myStaticAssert(sizeof(char) == 1);

/*
 * Allocation helpers (size and alignement constraints)
 */
static u64 getRealSizeOfRequest(u64 size) {
    if (size < sizeof(u64)) size = sizeof(u64);
    size = (size + ALIGNMENT - 1LL) & (~(ALIGNMENT-1LL));
    return size;
}


void* mallocProxyAllocate(
    ocrAllocator_t *self,   // Allocator to attempt block allocation (Not used, but exists to match other allocators)
    u64 size,               // Size of desired block, in bytes
    u64 hints)              // Allocator-dependent hints; not used for malloc. (Not used, but exists to match other allocators)
{
    DPRINTF(DEBUG_LVL_INFO, "mallocProxyAllocate called to get datablock of size %ld/0x%lx\n", (u64) size, (u64) size);
    u64 adjustedSize =
        getRealSizeOfRequest(size) +  // Client's requested size rounded up to alignment units, plus
        sizeof(blkHdr_t) +            // our block header size, plus
        ALIGNMENT - 1LL;              // enough to assure we can round up to an alignment boundary.
    u64 mallocAddr = (u64) malloc(adjustedSize);  // Call the system malloc
    if (mallocAddr == 0) return (void *) 0;
    u64 clientPayloadAddr = (mallocAddr + sizeof(blkHdr_t) + ALIGNMENT - 1LL) & (~(ALIGNMENT-1LL));
    blkHdr_t * blkHdr = (blkHdr_t *) (clientPayloadAddr - sizeof(blkHdr_t));
    VALGRIND_DEFINED(blkHdr);
    SET_distToMallocedBlock (blkHdr, (u8) (clientPayloadAddr-mallocAddr));
    SET_allocatorType (blkHdr, (u8) allocatorMallocProxy_id);
    setChecksum(blkHdr);
    VALGRIND_NOACCESS(blkHdr);
    DPRINTF(DEBUG_LVL_VERB, "mallocProxyAllocate got datablock at 0x%lx (system malloc returned 0x%lx)\n", (u64) clientPayloadAddr, (u64) mallocAddr);
#ifdef ENABLE_ALLOCATOR_INIT_NEW_DB_PAYLOAD
    u32 i;
    for (i = 0; i < size; i+= sizeof(u64)) {
        *((u64 *) (clientPayloadAddr + i)) = ENABLE_ALLOCATOR_INIT_NEW_DB_PAYLOAD;
    }
#endif
    return (void *) clientPayloadAddr;
}

void * mallocProxyReallocate(
    ocrAllocator_t *self,         // Allocator to attempt block allocation (Not used, but exists to match other allocators)
    void * origClientPayloadAddr, // Address of existing block.
    u64 size,                     // Size of desired block, in bytes
    u64 hints)                    // Allocator-dependent hints; not used for malloc. (Not used, but exists to match other allocators)
{
#if defined(ENABLE_ALLOCATOR_CHECKSUM) | \
    defined(ENABLE_ALLOCATOR_INIT_NEW_DB_PAYLOAD) | \
    defined(ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD) | \
    defined(ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS)
    // TODO:  At such time as this function is enabled for usage, deal with setting/checking checksums
    // and with the other ENABLE_ALLOCATOR... flags.  E.g., if the LEAK and the TRASH flags are set, it
    // would probably be appropriat to always do the realloc as a copy operation, and trash the old block
    static int warningGiven = 0;
    if (warningGiven == 0) {
        DPRINTF(DEBUG_LVL_WARN, "mallocProxyRealloc:  allocator checksuming, leaking, and/or payload [re]initing not implemented.\n");
        warningGiven = 1;
    }
    #endif
    DPRINTF(DEBUG_LVL_VERB, "mallocProxyRealloc called to resize 0x%lx\n to %ld/0x%lx\n",
        (u64) origClientPayloadAddr, (u64) size, (u64) size);
    size =
        getRealSizeOfRequest(size) +  // Client's requested size rounded up to alignment units, plus
        sizeof(blkHdr_t) +            // our block header size, plus
        ALIGNMENT - 1LL;              // enough to assure we can round up to an alignment boundary.
    blkHdr_t * blkHdr = (blkHdr_t *) (((u64) origClientPayloadAddr)-sizeof(blkHdr_t));
    VALGRIND_DEFINED(blkHdr);
    u64 mallocAddr = (u64) origClientPayloadAddr - GET_distToMallocedBlock(blkHdr);
    VALGRIND_NOACCESS(blkHdr);
    mallocAddr = (u64) realloc((void *) mallocAddr, size);
    if (mallocAddr == 0) return (void *) 0;
    u64 newClientPayloadAddr = (mallocAddr + sizeof(blkHdr_t) + ALIGNMENT - 1LL) & (~(ALIGNMENT-1LL));
    blkHdr = (blkHdr_t *) (newClientPayloadAddr - sizeof(blkHdr_t));
    VALGRIND_DEFINED(blkHdr);
    SET_distToMallocedBlock (blkHdr, (u8) (newClientPayloadAddr-mallocAddr));
    SET_allocatorType (blkHdr, (u8) allocatorMallocProxy_id);
    VALGRIND_NOACCESS(blkHdr);
    return (void *) newClientPayloadAddr;
}

void mallocProxyDeallocate(void * clientPayloadAddr)
{
#ifdef ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS
#define detail1 "LEAKED"
#else
#define detail1 " "
#endif
#ifdef ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD
#define detail2 "OVERWRITTEN"
#else
#define detail2 " "
#endif
    DPRINTF(DEBUG_LVL_INFO, "mallocProxyFree called to free 0x%lx, %s %s\n", (u64) clientPayloadAddr, detail1, detail2);
#ifdef ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD
    static bool firstMessage = true;
    if (firstMessage) {
        DPRINTF(DEBUG_LVL_WARN, "Can only trash the first quadword of datablock being freed on mallocProxy pool\n");
        DPRINTF(DEBUG_LVL_WARN, "because we do not know how large the datablock is\n");
        firstMessage = false;
    }
    *((u64 *) (clientPayloadAddr)) = ENABLE_ALLOCATOR_TRASH_FREED_DB_PAYLOAD;
#endif
    blkHdr_t * blkHdr = (blkHdr_t *) (((u64) clientPayloadAddr)-sizeof(blkHdr_t));
    VALGRIND_DEFINED(blkHdr);
    checkChecksum(blkHdr, __LINE__);
#ifndef ENABLE_ALLOCATOR_LEAK_FREED_DATABLOCKS
    u64 mallocAddr = (u64) clientPayloadAddr - GET_distToMallocedBlock(blkHdr);
    VALGRIND_NOACCESS(blkHdr);
    DPRINTF(DEBUG_LVL_VERB, "calling system free on 0x%lx\n", (u64) mallocAddr);
    free((void *) mallocAddr);
    DPRINTF(DEBUG_LVL_VERB, "back from system free on 0x%lx\n", (u64) mallocAddr);
#else
    VALGRIND_NOACCESS(blkHdr);
#endif
}


void mallocProxyDestruct(ocrAllocator_t *self) {
    // Destruct underlying memory-platform if any
    u64 i = 0;
    if (self->memoryCount != 0) {
        for(i=0; i < self->memoryCount; i++) {
            self->memories[i]->fcts.destruct(self->memories[i]);
        }
        runtimeChunkFree((u64)self->memories, NULL);
    }
    runtimeChunkFree((u64)self, NULL);
}

void mallocProxyBegin(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) {
    CHECK_AND_SET_MODULE_STATE(MALLOCPROXY-ALLOCATOR, self, BEGIN);
}

void mallocProxyStart(ocrAllocator_t *self, ocrPolicyDomain_t * PD ) {
    CHECK_AND_SET_MODULE_STATE(MALLOCPROXY-ALLOCATOR, self, START);
    // Get a GUID
    guidify(PD, (u64)self, &(self->fguid), OCR_GUID_ALLOCATOR);
    self->pd = PD;

    ASSERT(self->memoryCount == 1);

#if 0 // mallocProxy has no underlying memTarget or memPlatform.  The
      // one that is there is just a dummy because the machine-description
      // processing expects to stitch an allocator to a memTarget, and a
      // memTarget to a memPlatform, but we don't use them.  (If we decide
      // to eliminate the dummies, it saves memory on a platform where it
      // isn't really needed, and it perhaps makes the indices of remaining
      // allocators of other types (e.g. TLSF) to mismatch the indices of
      // memTargets and memPlatforms.  Seems not worthwhile.  But if we do
      // it, the above assert needs to change to ... == 0.
    self->memories[0]->fcts.start(self->memories[0], PD);
#endif
}

void mallocProxyStop(ocrAllocator_t *self) {
    CHECK_AND_SET_MODULE_STATE(MALLOCPROXY-ALLOCATOR, self, STOP);
    ocrPolicyMsg_t msg;
    getCurrentEnv(&(self->pd), NULL, NULL, &msg);

#if 0  // The memTarget (pointed to by self->memories[0]) is a dummy, so
       // maybe we shouldn't expect to do this statistics call.
#ifdef OCR_ENABLE_STATISTICS
    statsALLOCATOR_STOP(self->pd, self->guid, self, self->memories[0]->guid,
                        self->memories[0]);
#endif
#endif

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_DESTROY
    msg.type = PD_MSG_GUID_DESTROY | PD_MSG_REQUEST;
    PD_MSG_FIELD_I(guid) = self->fguid;
    PD_MSG_FIELD_I(properties) = 0;
    self->pd->fcts.processMessage(self->pd, &msg, false);
#undef PD_MSG
#undef PD_TYPE
    self->fguid.guid = NULL_GUID;
#if 0 // FIXME  -- I don't think we need this code:
#endif
}

void mallocProxyFinish(ocrAllocator_t *self) {
    CHECK_AND_SET_MODULE_STATE(MALLOCPROXY-ALLOCATOR, self, FINISH);
}

// Method to create the malloc allocator
ocrAllocator_t * newAllocatorMallocProxy(ocrAllocatorFactory_t * factory, ocrParamList_t *perInstance) {
    ocrAllocatorMallocProxy_t *result = (ocrAllocatorMallocProxy_t*)
        runtimeChunkAlloc(sizeof(ocrAllocatorMallocProxy_t), PERSISTENT_CHUNK);
    ocrAllocator_t * base = (ocrAllocator_t *) result;
    SET_MODULE_STATE(MALLOCPROXY-ALLOCATOR, base, NEW);
    factory->initialize(factory, base, perInstance);
    return (ocrAllocator_t *) result;
}

void initializeAllocatorMallocProxy(ocrAllocatorFactory_t * factory, ocrAllocator_t * self, ocrParamList_t * perInstance) {
    initializeAllocatorOcr(factory, self, perInstance);
}

/******************************************************/
/* OCR ALLOCATOR MALLOX FACTORY                       */
/******************************************************/

static void destructAllocatorFactoryMallocProxy(ocrAllocatorFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrAllocatorFactory_t * newAllocatorFactoryMallocProxy(ocrParamList_t *perType) {
    ocrAllocatorFactory_t* base = (ocrAllocatorFactory_t*)
        runtimeChunkAlloc(sizeof(ocrAllocatorFactoryMallocProxy_t), NONPERSISTENT_CHUNK);
    ASSERT(base);
    base->instantiate = &newAllocatorMallocProxy;
    base->initialize = &initializeAllocatorMallocProxy;
    base->destruct = &destructAllocatorFactoryMallocProxy;
    base->allocFcts.destruct = FUNC_ADDR(void (*)(ocrAllocator_t*), mallocProxyDestruct);
    base->allocFcts.begin = FUNC_ADDR(void (*)(ocrAllocator_t*, ocrPolicyDomain_t*), mallocProxyBegin);
    base->allocFcts.start = FUNC_ADDR(void (*)(ocrAllocator_t*, ocrPolicyDomain_t*), mallocProxyStart);
    base->allocFcts.stop = FUNC_ADDR(void (*)(ocrAllocator_t*), mallocProxyStop);
    base->allocFcts.finish = FUNC_ADDR(void (*)(ocrAllocator_t*), mallocProxyFinish);
    base->allocFcts.allocate = FUNC_ADDR(void* (*)(ocrAllocator_t*, u64, u64), mallocProxyAllocate);
    //base->allocFcts.free = FUNC_ADDR(void (*)(void*), mallocProxyDeallocate);
    base->allocFcts.reallocate = FUNC_ADDR(void* (*)(ocrAllocator_t*, void*, u64), mallocProxyReallocate);
    return base;
}
#endif /* ENABLE_MALLOCPROXY_ALLOCATOR */
