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
 * @brief This function should be called by the main user code
 * if present
 *
 * If a main function is defined, it should call ocrInit() to launch the runtime.
 * The mainEdt EDT will be launched immediately by the runtime
 *
 * If a main function is not defined, the OCR library defines one
 * which will immediately call the symbol 'mainEdt'
 *
 * This call is blocking until the runtime shuts down
 */
void ocrInit(int argc, char ** argv, ocrEdt_t mainEdt, bool createFinishEdt);

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
 * @brief Waits for an output event to be satisfied
 *
 * @warning This call is meant to be called from sequential C code
 * and is *NOT* supported on all implementations of OCR. This call runs
 * contrary to the 'non-blocking EDT' philosophy so use with care
 *
 * @param outputEvent       Event to wait on
 * @return A GUID to the data-block that was used to satisfy the
 * event
 */
ocrGuid_t ocrWait(ocrGuid_t outputEvent);

/**
 * @}
 **/
#endif /* __OCR_H__ */
