/**
 * @brief Macros for hardware-level primitives that the
 * rest of OCR uses. These macros are mostly related
 * to memory primitives
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#ifndef __OCR_HAL_X86_64_H__
#define __OCR_HAL_X86_64_H__

#include "ocr-types.h"
#include <string.h>
#include <stdlib.h>


/****************************************************/
/* OCR LOW-LEVEL MACROS                             */
/****************************************************/

/**
 * @brief Perform a memory fence
 *
 * @todo Do we want to differentiate different types
 * of fences?
 */

#define hal_fence() \
    do { __asm__ __volatile__("mfence":: : "memory"); } while(0);

/**
 * @brief Memory copy from source to destination
 *
 * @param destination[in]    A u64 pointing to where the data is to be copied
 *                           to
 * @param source[in]         A u64 pointing to where the data is to be copied
 *                           from
 * @param size[in]           A u64 indicating the number of bytes to be copied
 * @param isBackground[in]   A u8: A zero value indicates that the call will
 *                           return only once the copy is fully complete and a
 *                           non-zero value indicates the copy may proceed
 *                           in the background. A fence will then be
 *                           required to ensure completion of the copy
 * @todo Define what behavior we want for overlapping
 * source and destination
 */
#define hal_memCopy(destination, source, size, isBackground) \
    do { memcpy((void*)(destination), (const void*)(source), (size)); } while(0)
    
    
/**
 * @brief Compare and swap (64 bit)
 *
 * The semantics are as follows (all operations performed atomically):
 *     - if location is cmpValue, atomically replace with
 *       newValue and return cmpValue
 *     - if location is *not* cmpValue, return value at location
 *
 * @param atomic        u64*: Pointer to the atomic value (location)
 * @param cmpValue      u64: Expected value of the atomic
 * @param newValue      u64: Value to set if the atomic has the expected value
 *
 * @return Old value of the atomic
 */
#define hal_cmpswap64(atomic, cmpValue, newValue)                          \
    ({                                                                     \
        u64 tmp = 0;                                                       \
        __asm__ __volatile__("lock;\n"                                     \
                             "cmpxchg %1, %3"                              \
                             : "=rax" (tmp) /* %0 RAX, return value */     \
                             : "r" (newValue), /* %1 reg, new value */     \
                               "0" (cmpValue), /* %2 RAX, compare value */ \
                               "m" (*(atomic)) /* %3 mem, dest. operand */ \
                             : "memory", "cc" /* memory and cond reg */    \
            );                                                             \
        tmp;                                                               \
    })

/**
 * @brief Atomic add (64 bit)
 *
 * The semantics are as follows (all operations performed atomically):
 *     - atomically increment location by addValue
 *     - return new value (after addition)
 *
 * @param atomic    u64*: Pointer to the atomic value (location)
 * @param addValue  u64: Value to add to location
 * @return New value of the location
 */
#define hal_xadd64(atomic, addValue) ({ ASSERT(0); 0; })

/**
 * @brief Remote atomic add (64 bit)
 *
 * The semantics are as follows (all operations performed atomically):
 *     - atomically increment location by addValue
 *     - no value is returned (the increment will happen "at some
 *       point")
 *
 * @param atomic    u64*: Pointer to the atomic value (location)
 * @param addValue  u64: Value to add to location
 */
#define hal_radd64(atomic, addValue) { ASSERT(0); }

/**
 * @brief Compare and swap (32 bit)
 *
 * The semantics are as follows (all operations performed atomically):
 *     - if location is cmpValue, atomically replace with
 *       newValue and return cmpValue
 *     - if location is *not* cmpValue, return value at location
 *
 * @param atomic        u32*: Pointer to the atomic value (location)
 * @param cmpValue      u32: Expected value of the atomic
 * @param newValue      u32: Value to set if the atomic has the expected value
 *
 * @return Old value of the atomic
 */
#define hal_cmpswap32(atomic, cmpValue, newValue)                          \
    ({                                                                     \
        u32 tmp = 0;                                                       \
        __asm__ __volatile__("lock;\n"                                     \
                             "cmpxchg %1, %3"                              \
                             : "=eax" (tmp) /* %0 EAX, return value */     \
                             : "r" (newValue), /* %1 reg, new value */     \
                               "0" (cmpValue), /* %2 EAX, compare value */ \
                               "m" (*(atomic)) /* %3 mem, dest. operand */ \
                             : "memory", "cc" /* memory and cond reg */    \
            );                                                             \
        tmp;                                                               \
    })

/**
 * @brief Atomic add (32 bit)
 *
 * The semantics are as follows (all operations performed atomically):
 *     - atomically increment location by addValue
 *     - return new value (after addition)
 *
 * @param atomic    u32*: Pointer to the atomic value (location)
 * @param addValue  u32: Value to add to location
 * @return New value of the location
 */
#define hal_xadd32(atomic, addValue) ({ ASSERT(0); 0; })

/**
 * @brief Remote atomic add (32 bit)
 *
 * The semantics are as follows (all operations performed atomically):
 *     - atomically increment location by addValue
 *     - no value is returned (the increment will happen "at some
 *       point")
 *
 * @param atomic    u32*: Pointer to the atomic value (location)
 * @param addValue  u32: Value to add to location
 */
#define hal_radd32(atomic, addValue) ASSERT(0)

/**
 * @brief Convenience function that basically implements a simple
 * lock
 *
 * This will usually be a wrapper around cmpswap32. This function
 * will block until the lock can be acquired
 *
 * @param lock      Pointer to a 32 bit value
 */
#define hal_lock32(lock)                                \
    do {                                                \
        while(hal_cmpswap32((lock), 0, 1) != 0) ;       \
    } while(0)

/**
 * @brief Convenience function to implement a simple
 * unlock
 *
 * @param lock      Pointer to a 32 bit value
 */
#define hal_unlock32(lock) \
    do {                   \
        *(lock) = 0;       \
    } while(0)

/**
 * @brief Convenience function to implement a simple
 * trylock
 *
 * @param lock      Pointer to a 32 bit value
 * @return 0 if the lock has been acquired and a non-zero
 * value if it cannot be acquired
 */
#define hal_trylock32(lock) hal_cmpswap32(lock, 0, 1)

/**
 * @brief Abort the runtime
 *
 * Will crash the runtime
 */
#define hal_abort() abort()

/**
 * @brief Exit the runtime
 *
 * This will exit the runtime more cleanly than abort
 */
#define hal_exit(arg) exit(arg)
#endif /* __OCR_HAL_X86_64_H__ */

