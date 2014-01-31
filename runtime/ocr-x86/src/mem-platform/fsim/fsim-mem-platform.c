/**
 * @brief Simple implementation of a fsim wrapper
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "ocr-config.h"
#ifdef ENABLE_MEM_PLATFORM_FSIM

#include "ocr-hal.h"
#include "debug.h"
#include "utils/rangeTracker.h"
#include "mem-platform/fsim/fsim-mem-platform.h"
#include "ocr-mem-platform.h"
#include "ocr-sysboot.h"

#include <stdlib.h>

// Poor man's basic lock
#define INIT_LOCK(addr) do {*addr = 0;} while(0);
#define LOCK(addr) do { hal_lock32(addr); } while(0);
#define UNLOCK(addr) do { hal_unlock32(addr); } while(0);

/******************************************************/
/* OCR MEM PLATFORM FSIM IMPLEMENTATION             */
/******************************************************/

void fsimDestruct(ocrMemPlatform_t *self) {
    destroyRange(&(((ocrMemPlatformFsim_t*)self)->rangeTracker));
    runtimeChunkFree((u64)self, NULL);
}

struct _ocrPolicyDomain_t;

void fsimBegin(ocrMemPlatform_t *self, struct _ocrPolicyDomain_t * PD ) {
    // This is where we need to update the memory
    // using the sysboot functions
    self->startAddr = (u64)0xdeadbeef; // TODO: Bala to update this based on cfg file
    ASSERT(self->startAddr);
    self->endAddr = self->startAddr + self->size;

    ocrMemPlatformFsim_t *rself = (ocrMemPlatformFsim_t*)self;
    initializeRange(&(rself->rangeTracker), 16, self->startAddr,
                    self->endAddr, USER_FREE_TAG);
}

void fsimStart(ocrMemPlatform_t *self, struct _ocrPolicyDomain_t * PD ) {
    self->pd = PD;
    // Nothing much else to do
}

void fsimStop(ocrMemPlatform_t *self) {
    // Nothing to do
}

void fsimFinish(ocrMemPlatform_t *self) {
    ocrMemPlatformFsim_t *rself = (ocrMemPlatformFsim_t*)self;
    destroyRange(&(rself->rangeTracker));
    free((void*)self->startAddr);
}

u8 fsimGetThrottle(ocrMemPlatform_t *self, u64 *value) {
    return 1; // Not supported
}

u8 fsimSetThrottle(ocrMemPlatform_t *self, u64 value) {
    return 1; // Not supported
}

void fsimGetRange(ocrMemPlatform_t *self, u64* startAddr,
                    u64 *endAddr) {
    if(startAddr) *startAddr = self->startAddr;
    if(endAddr) *endAddr = self->endAddr;
}

u8 fsimChunkAndTag(ocrMemPlatform_t *self, u64 *startAddr, u64 size,
                     ocrMemoryTag_t oldTag, ocrMemoryTag_t newTag) {

    if(oldTag >= MAX_TAG || newTag >= MAX_TAG)
        return 3;
    
    ocrMemPlatformFsim_t *rself = (ocrMemPlatformFsim_t *)self;
    
    u64 iterate = 0;
    u64 startRange, endRange;
    u8 result;
    LOCK(&(rself->lock));
    do {
        result = getRegionWithTag(&(rself->rangeTracker), oldTag, &startRange,
                         &endRange, &iterate);
        if(endRange - startRange >= size) {
            // This is a fit, we do not look for "best" fit for now
            *startAddr = startRange;
            RESULT_ASSERT(splitRange(&(rself->rangeTracker),
                                     startRange, size, newTag), ==, 0);
            break;
        }
    } while(result == 0);

    UNLOCK(&(rself->lock));
    return result;
}

u8 fsimTag(ocrMemPlatform_t *self, u64 startAddr, u64 endAddr,
             ocrMemoryTag_t newTag) {

    if(newTag >= MAX_TAG)
        return 3;
    
    ocrMemPlatformFsim_t *rself = (ocrMemPlatformFsim_t *)self;

    LOCK(&(rself->lock));
    RESULT_ASSERT(splitRange(&(rself->rangeTracker), startAddr,
                             endAddr - startAddr, newTag), ==, 0);
    UNLOCK(&(rself->lock));
    return 0;
}

u8 fsimQueryTag(ocrMemPlatform_t *self, u64 *start, u64* end,
                  ocrMemoryTag_t *resultTag, u64 addr) {
    ocrMemPlatformFsim_t *rself = (ocrMemPlatformFsim_t *)self;

    RESULT_ASSERT(getTag(&(rself->rangeTracker), addr, start, end, resultTag),
                  ==, 0);
    return 0;
}

ocrMemPlatform_t* newMemPlatformFsim(ocrMemPlatformFactory_t * factory,
                                       u64 memSize, ocrParamList_t *perInstance) {

    // TODO: This will be replaced by the runtime/GUID meta-data allocator
    // For now, we cheat and use good-old fsim which is kind of counter productive with
    // all the trouble we are going through to *not* use fsim...
    ocrMemPlatform_t *result = (ocrMemPlatform_t*)
        runtimeChunkAlloc(sizeof(ocrMemPlatformFsim_t), NULL);

    result->pd = NULL;
    result->fcts = factory->platformFcts;
    result->size = memSize;
    result->startAddr = result->endAddr = 0ULL;
    ocrMemPlatformFsim_t *rself = (ocrMemPlatformFsim_t*)result;
    INIT_LOCK(&(rself->lock));
    return result;
}

/******************************************************/
/* OCR MEM PLATFORM FSIM FACTORY                    */
/******************************************************/

void destructMemPlatformFactoryFsim(ocrMemPlatformFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrMemPlatformFactory_t *newMemPlatformFactoryFsim(ocrParamList_t *perType) {
    ocrMemPlatformFactory_t *base = (ocrMemPlatformFactory_t*)
        runtimeChunkAlloc(sizeof(ocrMemPlatformFactoryFsim_t), (void *)1);
    
    base->instantiate = &newMemPlatformFsim;
    base->destruct = &destructMemPlatformFactoryFsim;
    base->platformFcts.destruct = FUNC_ADDR(void (*)(ocrMemPlatform_t*), fsimDestruct);
    base->platformFcts.begin = FUNC_ADDR(void (*)(ocrMemPlatform_t*, struct _ocrPolicyDomain_t*), fsimBegin);
    base->platformFcts.start = FUNC_ADDR(void (*)(ocrMemPlatform_t*, struct _ocrPolicyDomain_t*), fsimStart);
    base->platformFcts.stop = FUNC_ADDR(void (*)(ocrMemPlatform_t*), fsimStop);
    base->platformFcts.finish = FUNC_ADDR(void (*)(ocrMemPlatform_t*), fsimFinish);
    base->platformFcts.getThrottle = FUNC_ADDR(u8 (*)(ocrMemPlatform_t*, u64*), fsimGetThrottle);
    base->platformFcts.setThrottle = FUNC_ADDR(u8 (*)(ocrMemPlatform_t*, u64), fsimSetThrottle);
    base->platformFcts.getRange = FUNC_ADDR(void (*)(ocrMemPlatform_t*, u64*, u64*), fsimGetRange);
    base->platformFcts.chunkAndTag = FUNC_ADDR(u8 (*)(ocrMemPlatform_t*, u64*, u64, ocrMemoryTag_t, ocrMemoryTag_t), fsimChunkAndTag);
    base->platformFcts.tag = FUNC_ADDR(u8 (*)(ocrMemPlatform_t*, u64, u64, ocrMemoryTag_t), fsimTag);
    base->platformFcts.queryTag = FUNC_ADDR(u8 (*)(ocrMemPlatform_t*, u64*, u64*, ocrMemoryTag_t*, u64), fsimQueryTag);
    
    return base;
}

#endif /* ENABLE_MEM_PLATFORM_FSIM */
