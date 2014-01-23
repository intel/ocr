/**
 * @brief System boot dependences for fsim
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SYSBOOT_FSIM
#include "debug.h"

#include "ocr-mem-target.h"
#include "ocr-sysboot.h"

#define CHUNKSZ 32768                // The chunk size of runtimeChunkAlloc's pool
char persistent_chunk[CHUNKSZ];      // The underlying pool - persistent memory
char nonpersistent_chunk[CHUNKSZ];   // The underlying pool - non-persistent memory
u64  persistent_pointer = 0;         // The pointer to the free area of persistent
u64  nonpersistent_pointer = 0;      // The pointer to free area of non-persistent

u64 FsimRuntimeChunkAlloc(u64 size, u64 *extra) {
    void* returnValue = NULL;

    if(extra != NULL) { // default, non-persistent
        returnValue = &(nonpersistent_chunk[nonpersistent_pointer]);
        nonpersistent_pointer += size;
    } else {
        returnValue = &(persistent_chunk[persistent_pointer]);
        persistent_pointer += size;
    }

    ASSERT((persistent_pointer < CHUNKSZ) && "Persistent allocation needs more than CHUNKSZ bytes of memory");
    ASSERT((nonpersistent_pointer < CHUNKSZ) && "Non-persistent allocation needs more than CHUNKSZ bytes of memory");

    return (u64)returnValue;
}

void FsimRuntimeChunkFree(u64 addr, u64* extra) {
    ASSERT(addr);
    // Nothing to do
}

void FsimRuntimeUpdateMemTarget(ocrMemTarget_t *me, u64 extra) {
    // On Linux, we don't need to do anything since the mallocs never
    // overlap
    return;
}

void FsimBootUpAbort() {
 // TODO: Crash & burn
}

void FsimBootUpPrint(const char* str, u64 length) {
 // TODO
}

extern void *getAddress(const char *fname);

void *myGetFuncAddr (const char * fname) {
    return getAddress(fname);
}

void* (*getFuncAddr)(const char*) = &myGetFuncAddr;

u64 (*runtimeChunkAlloc)(u64, u64*) = &FsimRuntimeChunkAlloc;
void (*runtimeChunkFree)(u64, u64*) = &FsimRuntimeChunkFree;
void (*runtimeUpdateMemTarget)(ocrMemTarget_t *, u64) = &FsimRuntimeUpdateMemTarget;
void (*bootUpAbort)() = &FsimBootUpAbort;
void (*bootUpPrint)(const char*, u64) = &FsimBootUpPrint;

#endif /* ENABLE_SYSBOOT_FSIM */
