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
 * A macro to define a convenient way to initialize the OCR runtime.
 * The user needs to pass along the argc, argv arguments from the main function, as well as the list of pointers to all the functions
 * in the program that are used as EDTs
 */
#define OCR_INIT(argc, argv, ...)					\
    ({ocrEdt_t _fn_array[] = {__VA_ARGS__}; ocrInit(argc, argv, sizeof(_fn_array)/sizeof(_fn_array[0]), _fn_array);})

/**
 * @brief Initial function called by the "main" program to set-up the OCR
 * environment.
 *
 * This call should be paired with an ocrCleanup call
 */
void ocrInit(int * argc, char ** argv, u32 fnc, ocrEdt_t funcs[] );

/**
 * @brief Called by an EDT to indicate the end of an OCR
 * execution
 *
 * This call will cause the OCR runtime to terminate
 */
void ocrFinish();

/**
 * Called by the last executing EDT to signal the runtime
 * the user program is done.
 *
 * Warning: ocr_finalize() must be called at the end of the program !
 */
/**
 * @brief Called by the "main" program to clean-up the OCR
 * environment
 *
 * @note This call *waits* for the ocrFinish function to be called
 * before completing. In other words, time-wise, ocrInit will be called,
 * then ocrCleanup which will wait until ocrFinish is called and then
 * ocrCleanup will finish cleaning up and control will be returned
 * to the main program
 */
void ocrCleanup();

#endif /* __OCR_H__ */
