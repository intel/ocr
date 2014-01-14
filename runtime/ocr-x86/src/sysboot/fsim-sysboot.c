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

#include <stdlib.h>
#include <stdio.h>

#define CHUNKSZ 65536        // The chunk size of runtimeChunkAlloc's pool
char *bigchunk = NULL;       // The underlying pool
int pointer = 0;             // The pointer to the free area

u64 FsimRuntimeChunkAlloc(u64 size, u64 *extra) {
    void* returnValue = NULL;

    if(bigchunk == NULL) {
        bigchunk = calloc(1, CHUNKSZ);
        ASSERT(bigchunk && "Unable to allocate chunk heap");
        pointer = 0;
    }

    returnValue = &(bigchunk[pointer]);

    //printf("Allocing %ld bytes for %s at %lx\n", size, (char *)extra, (u64)returnValue);
    pointer += size;
    ASSERT(pointer < CHUNKSZ && "Allocation needs more than CHUNKSZ bytes of memory");
    return (u64)returnValue;
}

void FsimRuntimeChunkFree(u64 addr, u64* extra) {
    ASSERT(addr);
    free((void*)addr);
}

void FsimRuntimeUpdateMemTarget(ocrMemTarget_t *me, u64 extra) {
    // On Linux, we don't need to do anything since the mallocs never
    // overlap
    return;
}

void *myGetFuncAddr (const char * fname) {
    // TODO: Call into elf_utils to extract the function from a specified binary
    return (void *)0xdeadc0de;
}

void* (*getFuncAddr)(const char*) = &myGetFuncAddr;

u64 (*runtimeChunkAlloc)(u64, u64*) = &FsimRuntimeChunkAlloc;
void (*runtimeChunkFree)(u64, u64*) = &FsimRuntimeChunkFree;
void (*runtimeUpdateMemTarget)(ocrMemTarget_t *, u64) = &FsimRuntimeUpdateMemTarget;

#endif /* ENABLE_SYSBOOT_FSIM */
