/**
 * @brief System boot dependences for x86
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"

#include "ocr-sysboot.h"

#include <stdlib.h>

u64 x86RuntimeChunkAlloc(u64 size, u64 *extra) {
    void* returnValue = malloc(size);
    ASSERT(returnValue);
    return (u64)returnValue;
}

void x86RuntimeChunkFree(u64 addr, u64* extra) {
    ASSERT(addr);
    free((void*)addr);
}

u64 (*runtimeChunkAlloc)(u64, u64*) = &x86RuntimeChunkAlloc;
void (*runtimeChunkFree)(u64, u64*) = &x86RuntimeChunkFree;
