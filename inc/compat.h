#ifndef __COMPAT_H__
#define __COMPAT_H__

#if defined(__OCR__)

#include "ocr.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define PTR_T     void*

#define MALLOC(a) malloc(a)
#define FREE(a)   free(a)

#undef ASSERT
#define ASSERT(val) assert(val)

#undef PRINTF
#define PRINTF(format, ...) fprintf(stderr, format, ## __VA_ARGS__)

#define DBCREATE(guid, addr, size, flags, affinity, alloc) \
    do {                                                   \
        ocrDbCreate((guid), (addr), (size),                \
            (flags), (affinity), (alloc));                 \
    } while(0);

#define GUIDTOU64(guid) (u64)(guid)

#include <math.h>

#elif defined(__FSIM__)

#include "xe-edt.h"
#include "xe-memory.h"

#define PTR_T     u64

#define MALLOC(a) xeMalloc(a)
#define FREE(a)   xeFree(a)

#undef ASSERT
#define ASSERT(val) if(!(val)) {                                        \
        __asm__ __volatile__ __attribute__ (( noreturn )) (             \
            "movimmz r2, %1\n\t"                                        \
            "alarm %2\n\t"                                              \
            :                                                           \
            : "{r3}" (__FILE__), "L" (__LINE__), "L" (0xC1)             \
            : "r2"                                                      \
            );                                                          \
    }

#undef PRINTF
#define PRINTF(format, ...) xe_printf(format, ## __VA_ARGS__)

#define DBCREATE(guid, addr, size, flags, affinity, alloc) \
    do {                                                   \
        ocrGuid_t _affinity;                               \
        ocrLocation_t _allocLocation;                      \
        _allocLocation.type = OCR_LOC_TYPE_RELATIVE;       \
        _allocLocation.data.relative.identifier = 0;       \
        _allocLocation.data.relative.level =               \
            OCR_LOCATION_DRAM;                             \
        ocrGuidSetAddressU64(&(_affinity),                 \
            (u64)&_allocLocation);                         \
        ocrDbCreate((guid), (addr), (size),                \
            (flags), (_affinity), (alloc));                \
    } while(0);

#define GUIDTOU64(guid) (u64)((guid).data)

static double sqrt(double value) {
    double result;
    __asm__ (
            "rcpsqrtF64 %0, %1"
            : "=r" (result)
            : "r" (value)
            );
    return 1/result;
}
#endif /* __FSIM__ */

#endif /* __COMPAT_H__ */
