/**
 * @brief Top level OCR library file. Include this file in any
 * program making use of OCR. You do not need to include any other
 * files.
 * @authors Romain Cledat, Intel Corporation
 * @date 2012-09-21
 * Copyright (c) 2012, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of Intel Corporation nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#ifndef __OCR_H__
#define __OCR_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "ocr-types.h"
#include "ocr-db.h"
#include "ocr-edt.h"
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
