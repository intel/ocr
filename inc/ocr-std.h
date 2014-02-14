/**
 * @brief Limited "standard" API for OCR
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_STD_H__
#define __OCR_STD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ocr-types.h"


/**
 * @defgroup OCRStandrdAPI Limited "standrd" API for OCR
 * @brief Describes the limited "standard" API for OCR
 *
 * Standard APIs like PRINTF(), potentially limited file I/O,
 * etc. Very limited at this stage but may be extended as needed and
 * as possible later.
 *
 * @{
 **/


/**
 * @brief Console output
 *
 * This maps to the typical C-like printf() functionality.
 *
 * @param fmt       Format specifier, as per printf().
 * @param ...       Arguments, as per printf().
 *
 * @return Number of characters printed, as per printf().
 *
 **/
u32 PRINTF(char * fmt, ...);


/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __OCR_STD_H__ */
