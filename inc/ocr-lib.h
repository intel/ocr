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

#ifndef __OCR_LIB_H__
#define __OCR_LIB_H__

#include "ocr-db.h"


/**
 * @defgroup OCRLibrary OCR used as a Library
 * @brief Describes the OCR as library API
 *
 * When OCR is used as a library, the user need to
 * explicitly bring up and tear down the runtime.
 *
 * @{
 **/

/**
 * @brief Data-structure holding configuration elements for the runtime
 *
 * Members of the struct can be filed by calling ocrParseArgs or be manually set.
 */
typedef struct _ocrConfig_t {
    int userArgc;
    char ** userArgv;
    const char * iniFile;
} ocrConfig_t;

/**
 * @brief OCR library mode - Initialize OCR
 * @param ocrConfig The configuration to use to bring this OCR instance up.
 */
void ocrInit(ocrConfig_t * ocrConfig);

/**
 * @brief OCR library mode - Parse arguments and extract OCR options
 * @param argc The number of elements in argv.
 * @param argv Array of char * argumetns.
 * @param ocrConfig Pointer to an ocrConfig ocrParseArgs will populate.
 * Warning: Removes OCR parameters from argv and pack elements.
 */
void ocrParseArgs(int argc, const char* argv[], ocrConfig_t * ocrConfig);

/**
 * @brief OCR library mode - Terminates OCR execution
 * This call bring down the runtime after ocrShutdown has been called by an EDT.
 */
void ocrFinalize();

/**
 * @brief OCR library mode - Waits for an output event to be satisfied
 *
 * @warning This call is meant to be called from sequential C code
 * and is *NOT* supported on all implementations of OCR. This call runs
 * contrary to the 'non-blocking EDT' philosophy so use with care
 *
 * @param outputEvent       Event to wait on
 * @return A GUID to the data-block that was used to satisfy the event
 */
ocrGuid_t ocrWait(ocrGuid_t outputEvent);

/**
 * @}
 */

#endif /* __OCR_LIB_H__ */
