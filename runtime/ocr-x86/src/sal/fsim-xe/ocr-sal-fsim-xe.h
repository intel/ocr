/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_SAL_FSIM_XE_H__
#define __OCR_SAL_FSIM_XE_H__

#include "ocr-hal.h"
#include "xe-abi.h"

#define sal_abort() hal_abort()

#define sal_exit(x) hal_exit(x)

#define sal_assert(x, fn, ln) if(!(x)) {                            \
        __asm__ __volatile__ __attribute__ (( noreturn )) (         \
            "alarm %2\n\t"                                          \
            : : "{r2}" (ln), "{r3}" (fn), "L" (XE_ASSERT));         \
    }

#define sal_print(msg, len) __asm__ __volatile__(                   \
        "alarm %2\n\t"                                              \
        : : "{r2}" (msg), "{r3}" (len), "L" (XE_CONOUT))

#endif /* __OCR_SAL_FSIM_XE_H__ */
