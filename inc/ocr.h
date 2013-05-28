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

#include "ocr-types.h"
#include "ocr-db.h"
#include "ocr-edt.h"

/**
 * @defgroup OCRBoot Initialization and cleanup routines for OCR
 * @brief Describes how to instantiate and tear-down the runtime
 *
 * @{
 **/

/**
 * @brief A convenience macro to initialize the runtime
 *
 * This macro will initialize the runtime and call the first 'main' EDT to be
 * invoked with the provided 'argc' and 'argv' argument. The user must also
 * provide all root C functions that will be called as EDTs
 *
 * @note 'argc' and 'argv' include both the arguments to be passed to
 * the main EDT as well as the OCR runtime arguments (if any). 'argc' and
 * 'argv' will be processed to remove OCR runtime arguments before they are
 * passed to the main EDT.
 *
 * @param argc      Pointer to the number of arguments (OCR + main EDT)
 * @param argv      Array of arguments (argc count)
 * @param ...       Function pointers that will be used as EDTs
 **/
#define OCR_INIT(argc, argv, ...)					\
    ({ocrEdt_t _fn_array[] = {__VA_ARGS__}; ocrInit(argc, argv, sizeof(_fn_array)/sizeof(_fn_array[0]), _fn_array);})

/**
 * @brief Initial function called by the C program to set-up the OCR
 * environment.
 *
 * This call is called by #OCR_INIT. It should be paired with
 * an ocrCleanup() call
 *
 * @param argc      Pointer to the number of arguments (OCR + main EDT)
 * @param argv      Array of arguments (argc count)
 * @param fnc       Number of function pointers (UNUSED)
 * @param funcs     Array of function pointers to be used as EDTs (UNUSED)
 *
 **/
void ocrInit(int * argc, char ** argv, u32 fnc, ocrEdt_t funcs[] );

/**
 * @brief Called by an EDT to indicate the end of an OCR
 * execution
 *
 * This should be called by the last EDT to execute (it is
 * up to the user to determine this). It should only be called
 * once and will call the runtime to be torn down
 *
 */
void ocrFinish();

/**
 * @brief Called by the 'main' program to clean-up the OCR
 * environment
 *
 * This call is the symmetric of ocrInit() and *waits* for the ocrFinish()
 * function to be called before completing. In other words, time-wise,
 * ocrInit() will be called, then ocrCleanup() which will wait until
 * ocrFinish() is called and then
 * ocrCleanup() will finish cleaning up and control will be returned
 * to the main program
 */
void ocrCleanup();

/**
 * @}
 **/
#endif /* __OCR_H__ */
