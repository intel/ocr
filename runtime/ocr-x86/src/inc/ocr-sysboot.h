/**
 * @brief System boot dependences
 *
 * This file describes some of the functions that the runtime depends
 * on to bring up the runtime (ie: what is available and needed *BEFORE*
 * the runtime is brought up).
 *
 * Specifically, when the runtime starts running, it will build up a
 * policy domain and any accompanying data structures, during this time
 * (nothing really exists), the runtime relies on the functions
 * in this file. Once the policy domain is fully built-out, the function
 * 'start' is called on the policy domain which will (at some point) call
 * 'runtimeUpdateMemTarget'. Once the policy domain is up and running, these
 * functions are never called again.
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

/**
 * @brief Modifies the memory target mem to reflect the space potentially
 * used by runtimeChunkAlloc and runtimeChunkFree.
 *
 * This function will be called as the runtime is bringing itself up: when the
 * policy domain is started, this will trickle down and start the memory targets. As
 * they start, this function will be called to modify them prior to starting
 * if required.
 *
 * @param mem[in]           Memory target to update
 * @param extra[in/out]     Implementation specific value (if needed)
 */
extern void (*runtimeUpdateMemTarget)(struct _ocrMemTarget_t *mem, u64 extra);

/**
 * @brief Function called to temporarily set the answer of getCurrentPD and
 * to something sane when the workers have not
 * yet been brought up
 *
 * This order of boot-up is as follows:
 *     - Bring up the factories needed to build a policy domain
 *     - Bring up a GUID provider
 *     - Build a policy domain (the root one)
 *     - Call setBootPD
 *     - Bring up all other instances and do any mapping needed
 *     - Call setIdentifyingFunctions on the comp platform factory to reset
 *       the getCurrentPD etc properly
 *
 * @param domain    Domain to use as the boot PD
 * @return nothing
 */
extern void (*setBootPD)(struct _ocrPolicyDomain_t* domain);

#endif /* __OCR_SYSBOOT_H__ */
