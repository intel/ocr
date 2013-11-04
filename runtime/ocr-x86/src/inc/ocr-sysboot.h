/**
 * @brief System boot dependences
 *
 * This file describes some of the functions that the runtime depends
 * on to bring up the runtime (ie: what is available and needed *BEFORE*
 * the runtime is brought up)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_SYSBOOT_H__
#define __OCR_SYSBOOT_H__

#include "ocr-types.h"

/**
 * @brief "Allocates" a chunk of memory for the runtime of size 'size'
 *
 * This may or may not be a real allocation in the sense that allocators
 * may not exist at this point. Anything "allocated" with this call
 * may or may not be freeable so it should be used only for persistent
 * data structures (such as the structures that define the runtime).
 *
 * The runtime will assume that this function pointer is properly
 * initialized when it tries to boot.
 *
 * @param size[in]          Size to allocate
 * @param extra[in/out]     Implementation specific value (if needed)
 * @return NULL or an address in memory
 */
extern u64 (*runtimeChunkAlloc)(u64 size, u64 *extra);

/**
 * @brief "Frees" a chunk of memory for the runtime
 *
 * This function may or may not be implemented and should ideally
 * only be called on runtime tear-down
 *
 * The runtime will assume that this function pointer is properly
 * initialized when it tries to boot.
 *
 * @param addr[in]          Location to free
 * @param extra[in/out]     Implementation specific value (if needed)
 */
extern void (*runtimeChunkFree)(u64 addr, u64 *extra);

#endif /* __OCR_SYSBOOT_H__ */
