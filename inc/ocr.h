/**
 * @brief Top level OCR library file. Include this file in any
 * program making use of OCR. You do not need to include any other
 * files unless using extended or experimental features present in the
 * extensions/ folder
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
#include "ocr-std.h"

/**
 * @defgroup OCRGeneral Support calls for OCR
 * @brief Describes general support functions for OCR
 *
 * @{
 */
/**
 * @brief Called by an EDT to indicate the end of an OCR
 * program
 *
 * This call will cause the OCR runtime to shutdown
 *
 * @note If using the extended ocrWait() call present in ocr-legacy.h,
 * you do not need to use ocrShutdown() as the termination of the
 * OCR portion will be captured in the finish EDT's return
  */
void ocrShutdown();

/**
 * @brief Called by an EDT to indicate an abnormal end of an
 * OCR program
 *
 * This call will cause the OCR runtime to shutdown with an
 * error code. Calling this with 0 as an argument is
 * equivalent to ocrShutdown().
 *
 * @param errorCode    User defined error code returned to
 *                     the environment
 */
void ocrAbort(u8 errorCode);

/**
 * @brief Retrieves the traditional 'argc' value in mainEdt.
 *
 * When starting, the first EDT (called mainEdt) gets a single data block that contains
 * all of the arguments passed to the program. These arguments are packed in the
 * following format:
 *     - first 8 bytes: argc
 *     - argc count of 8 byte values indicating the offsets from
 *       the start of the data block to the argument (ie: the first
 *       8 byte value indicates the offset in bytes to the first argument)
 *     - the arguments (NULL terminated character arrays)
 *
 * This call will extract the number of arguments (argc)
 *
 * @param dbPtr    Pointer to the start of the argument data block
 * @return         Number of arguments
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

