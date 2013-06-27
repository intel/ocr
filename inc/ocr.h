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
#define OCR_INIT(argc, argv, mainEdt)					\
    do { ocrInit(argc, argv, mainEdt); } while(0);

/**
 * @brief Initial function called by the C program to set-up the OCR
 * environment.
 *
 * This call should be paired with an ocrCleanup call
 */
void ocrInit(int * argc, char ** argv, ocrEdt_t mainEdt);

/**
 * @brief Called by an EDT to indicate the end of an OCR
 * execution
 *
 * This should be called by the last EDT to execute (it is
 * up to the user to determine this). It should only be called
 * once and will call the runtime to be torn down
 *
 */
void ocrFinalize();

/**
 * @}
 **/
#endif /* __OCR_H__ */
