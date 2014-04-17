/**
 * @brief Basic types used throughout the OCR library
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_TYPES_H__
#define __OCR_TYPES_H__

#include <stddef.h>
#include <stdint.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  s64;
typedef int32_t  s32;
typedef int8_t   s8;

/* boolean support in C */
#ifndef __cplusplus
#define true 1
#define TRUE 1
#define false 0
#define FALSE 0
typedef u8 bool;
#endif /* __cplusplus */
#ifdef __cplusplus
#define TRUE true
#define FALSE false
extern "C" {
#endif

/**
 * @brief Type describing the unique identifier of most
 * objects in OCR (EDTs, data-blocks, etc).
 **/
typedef intptr_t ocrGuid_t; /**< GUID type */
//typedef u64 ocrGuid_t;

/**
 * @brief An invalid ocrGuid_t
 */
#define NULL_GUID ((ocrGuid_t)0x0)

/**
 * @brief Allocators that can be used to allocate
 * within a data-block
 */
typedef enum {
    NO_ALLOC = 0 /**< No allocation inside data-blocks */
    /* Add others */
} ocrInDbAllocator_t;

#ifdef __cplusplus
}
#endif
#endif /* __OCR_TYPES_H__ */
