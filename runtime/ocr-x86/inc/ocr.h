/**
 * @brief Top level OCR library file. Include this file in any
 * program making use of OCR. You do not need to include any other
 * files.
 */
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_H__
#define __OCR_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "ocr-db.h"
#include "ocr-edt.h"
#include "ocr-errors.h"
#include "ocr-types.h"
#include "compat.h"

/**
 * @brief Called by an EDT to indicate the end of an OCR
 * execution
 *
 * This call will cause the OCR runtime to shutdown
 *
 * @note This call is not necessarily required if using ocrWait on
 * a finish EDT from a sequential C code (ie: the ocrShutdown call will
 * implicitly be encapsulated in the fact that the finish EDT returns)
 */
void ocrShutdown();

/**
 * @brief Gets the number of arguments packed in the
 * single data-block passed to the first EDT
 *
 * When starting, the first EDT gets passed a single data-block that contains
 * all the arguments passed to the program. These arguments are packed in the
 * following format:
 *     - first 8 bytes: argc
 *     - argc count of 8 byte values indicating the offsets from
 *       the start of the data-block to the argument (ie: the first
 *       8 byte value indicates the offset in bytes to the first argument)
 *     - the arguments (NULL terminated character arrays)
 *
 * This call will extract the number of arguments
 *
 * @param dbPtr    Pointer to the start of the argument data-block
 * @return Number of arguments
 */
u64 getArgc(void* dbPtr);

/**
 * @brief Gets the argument 'count' from the data-block containing the
 * arguments
 *
 * @see getArgc() for an explanation
 *
 * @param dbPtr    Pointer to the start of the argument data-block
 * @param count    Index of the argument to extract
 * @return A NULL terminated string
 */
char* getArgv(void* dbPtr, u64 count);
#ifdef __cplusplus
}
#endif
/**
 * @}
 **/
#endif /* __OCR_H__ */
