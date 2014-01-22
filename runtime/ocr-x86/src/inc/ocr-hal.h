/**
 * @brief Hardware Abstraction layer
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_HAL_H__
#define __OCR_HAL_H__

#include "ocr-config.h"

#if defined(HAL_XE)
#include "hal/xe/ocr-hal-xe.h"
#elif defined(HAL_X86_64)
#include "hal/x86_64/ocr-hal-x86_64.h"
#else
#error "Unknown HAL layer"
#endif /* Switch of HAL_LAYER */

#endif /* __OCR_HAL_H__ */
