/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_SAL_FSIM_CE_H__
#define __OCR_SAL_FSIM_CE_H__

#include "ocr-hal.h"

#define sal_abort()   hal_abort()

#define sal_exit(x)   hal_exit(x)

#define sal_assert(x, f, l)   if(!(x)) hal_abort()

#define sal_print(msg, len) __asm__ __volatile__("int $0xFF\n\t" : : "a" (msg))

#endif /* __OCR_SAL_FSIM_CE_H__ */
