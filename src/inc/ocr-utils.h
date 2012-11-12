/* Copyright (c) 2012, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __OCR_UTILS_H__
#define __OCR_UTILS_H__

#include "ocr-types.h"

/******************************************************/
/* LOGGING FACILITY                                   */
/******************************************************/

/*
 * Default logging output to stdout
 */
#define ocr_log_print(...) fprintf(stdout, __VA_ARGS__);

/*
 * List of components you can enable / disable logging
 */
#define LOGGER_WORKER 1

/*
 * Loggers levels
 */
#define LOG_LEVEL_INFO 1

/*
 * Current logging level
 */
#define LOG_LEVEL_CURRENT 0

#define ocr_log(type, level, fmt, ...) \
    if ((LOGGER_ ## type) && ((LOG_LEVEL_ ## level) <= LOG_LEVEL_CURRENT)) \
        { ocr_log_print(fmt, __VA_ARGS__);}

/*
 * Convenience macro to log worker-related events
 */
#define log_worker(level, fmt, ...) ocr_log(WORKER, level, fmt, __VA_ARGS__)

/******************************************************/
/*  ABORT / EXIT OCR                                  */
/******************************************************/

void ocr_abort();

void ocr_exit();

/**
 * @brief Bit operations used to manipulate bit
 * vectors. Currently used in the regular
 * implementation of data-blocks
 */

/**
 * @brief Finds the position of the MSB that
 * is 1
 *
 * @param val  Value to look at
 * @return  MSB set to 1 (from 0 to 15)
 */
u32 fls16(u16 val);

/**
 * @brief Finds the position of the MSB that
 * is 1
 *
 * @param val  Value to look at
 * @return  MSB set to 1 (from 0 to 31)
 */
u32 fls32(u32 val);

/**
 * @brief Finds the position of the MSB that
 * is 1
 *
 * @param val  Value to look at
 * @return  MSB set to 1 (from 0 to 63)
 */
u32 fls64(u64 val);


#endif /* __OCR_UTILS_H__ */
