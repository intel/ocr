/**
 * @brief System boot dependences for fsim-builder
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_BUILDER_ONLY
#include "debug.h"

#include "ocr-mem-target.h"
#include "ocr-sysboot.h"

#define CHUNKSZ 32768                // The chunk size of runtimeChunkAlloc's pool
char persistent_chunk[CHUNKSZ];      // The underlying pool - persistent memory
char nonpersistent_chunk[CHUNKSZ];   // The underlying pool - non-persistent memory
char args_chunk[CHUNKSZ];            // The underlying pool - for args
u64  persistent_pointer = 0;         // The pointer to the free area of persistent
u64  nonpersistent_pointer = 0;      // The pointer to free area of non-persistent
u64  args_pointer = 0;               // The pointer to free area of args pool

#define DEBUG_TYPE SYSBOOT

static void * packedUserArgs = NULL;
static ocrEdt_t mainEdt = NULL;

u64 FsimRuntimeChunkAlloc(u64 size, u64 *extra) {
    void* returnValue = NULL;
    if(extra==PERSISTENT_CHUNK) {
        // Align the offset if unaligned
        u64 offset = (u64)&(persistent_chunk[persistent_pointer]);
        if(offset % 8) persistent_pointer += (8 - (offset%8));
        returnValue = &(persistent_chunk[persistent_pointer]);
        persistent_pointer += size;
    } else if(extra == ARGS_CHUNK) { // special case, for arguments
        returnValue = &(args_chunk[args_pointer]);
        args_pointer += size;
    } else { // default, non-persistent
        returnValue = &(nonpersistent_chunk[nonpersistent_pointer]);
        nonpersistent_pointer += size;
    }

    ASSERT((persistent_pointer < CHUNKSZ) && "Persistent allocation needs more than CHUNKSZ bytes of memory");
    ASSERT((nonpersistent_pointer < CHUNKSZ) && "Non-persistent allocation needs more than CHUNKSZ bytes of memory");
    ASSERT((args_pointer < CHUNKSZ) && "Non-persistent allocation needs more than CHUNKSZ bytes of memory");

    DPRINTF(DEBUG_LVL_VVERB, "Runtime chunk alloc(%lld, %lld) => 0x%llx\n", size, (u64)extra, (u64)returnValue);

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

void fsimMainEdtSet(ocrEdt_t edt) {
    return;
}

void fsimUserArgsSet(void *ptr) {
    packedUserArgs = ptr;
}

void* fsimUserArgsGet(void) {
    return packedUserArgs;
}

void (*userArgsSet)(void *) = &fsimUserArgsSet;
void * (*userArgsGet)() = &fsimUserArgsGet;
void (*mainEdtSet)(ocrEdt_t) = &fsimMainEdtSet;
ocrEdt_t (*mainEdtGet)() = NULL;

void* (*getFuncAddr)(const char*) = &myGetFuncAddr;

u64 (*runtimeChunkAlloc)(u64, u64*) = &FsimRuntimeChunkAlloc;
void (*runtimeChunkFree)(u64, u64*) = &FsimRuntimeChunkFree;
void (*runtimeUpdateMemTarget)(ocrMemTarget_t *, u64) = &FsimRuntimeUpdateMemTarget;
void (*bootUpAbort)() = &FsimBootUpAbort;
void (*bootUpPrint)(const char*, u64) = &FsimBootUpPrint;

#endif /* ENABLE_SYSBOOT_FSIM */
