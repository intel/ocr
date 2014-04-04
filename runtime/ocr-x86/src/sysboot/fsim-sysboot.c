/**
 * @brief System boot dependences for fsim - empty because there should be none
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

u64 FsimRuntimeChunkAlloc(u64 size, u64 *extra) {
    ASSERT(0);
    return (u64)0;
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

void *myGetFuncAddr (const char * fname) {
    return NULL;
}

void* (*getFuncAddr)(const char*) = &myGetFuncAddr;

void (*userArgsSet)(void *) = NULL;
void * (*userArgsGet)() = NULL;
void (*mainEdtSet)(ocrEdt_t) = NULL;
ocrEdt_t (*mainEdtGet)() = NULL;
u64 (*runtimeChunkAlloc)(u64, u64*) = &FsimRuntimeChunkAlloc;
void (*runtimeChunkFree)(u64, u64*) = &FsimRuntimeChunkFree;
void (*runtimeUpdateMemTarget)(ocrMemTarget_t *, u64) = &FsimRuntimeUpdateMemTarget;
void (*bootUpAbort)() = &FsimBootUpAbort;
void (*bootUpPrint)(const char*, u64) = &FsimBootUpPrint;

#endif /* ENABLE_SYSBOOT_FSIM */
