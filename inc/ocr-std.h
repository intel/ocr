/**
 * @brief Limited I/O API for OCR
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_IO_H__
#define __OCR_IO_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ocr-types.h"


/**
 * @defgroup OCRInputOutput Limited I/O API for OCR
 * @brief Describes the limited I/O API for OCR
 *
 * I/O capabilities are very limited at this stage but may be extended
 * as needed and possible later. Currently, we only support console
 * output, but may provide some restricted form of 'file' I/O later.
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

#endif /* __OCR_IO_H__ */
