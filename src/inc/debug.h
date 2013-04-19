/**
 * @brief Some debug utilities for OCR
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



#ifndef __DEBUG_H__
#define __DEBUG_H__

// #include <pthread.h>
// #include <time.h>
#include <stdio.h>
#include <assert.h>
// #include <stdarg.h>

#define PRINTF printf

#ifdef OCR_DEBUG

/**
 * @brief No debugging messages are printed
 *
 * Note that you can still be in debugging mode
 * and have no messages printed as this would allow
 * all ASSERTs to be checked
 */
#define DEBUG_LVL_NONE      0

/**
 * @brief Only warnings are printed
 */
#define DEBUG_LVL_WARN      1

/**
 * @brief Warnings and informational
 * messages are printed
 *
 * Default debug level if nothing is specified
 * for #DEBUG_RUNTIME_LEVEL (paramater defined through compilation)
 */
#define DEBUG_LVL_INFO      2
/**
 * @brief Warnings, informational
 * messages and verbose messages are printed
 */
#define DEBUG_LVL_VERB      3

/**
 * @brief Everything is printed
 */
#define DEBUG_LVL_VVERB     4

#ifndef OCR_DEBUG_LEVEL
/**
 * @brief Debug level
 *
 * This controls the verbosity of the
 * debug messages in debug mode
 */
#define OCR_DEBUG_LEVEL DEBUG_LVL_INFO
#endif

// Imply OCR_STATUS
#define OCR_STATUS
// Imply ASSERTs
#define OCR_ASSERT

#define DO_DEBUG(level) if(level <= OCR_DEBUG_LEVEL)

// #define TIMING_CLOCK CLOCK_REALTIME

/* #define DEBUG(level, format , ...)                                   \
     if(level <= DEBUG_LEVEL) {                                          \
         struct timespec __curtime;                                      \
         clock_gettime(TIMING_CLOCK, &__curtime);                        \
         fprintf(stderr, "\t--%ld.%ld T %p %s:%d "  format, __curtime.tv_sec, __curtime.tv_nsec, (void*)pthread_self(), __FILE__, __LINE__,##__VA_ARGS__); \
     }
*/

#else
#define DO_DEBUG(level) if(0)

#endif /* OCR_DEBUG */


#ifdef OCR_STATUS
#define STATUS(format, ...)                                             \
    PRINTF("##OCR-STATUS %s:%d " format, __FILE__, __LINE__,##__VA_ARGS__);
#else
#define STATUS(format, ...)
#endif /* OCR_STATUS */

#define STATUS0(format) STATUS("%s" format, "")

#ifdef OCR_ASSERT
#define ASSERT(a) do { assert(a); } while(0);
#define RESULT_ASSERT(a, op, b) do { assert((a) op (b)); } while(0);
#define RESULT_TRUE(a) do { assert(a); } while(0);
#else
#define ASSERT(a)
#define RESULT_ASSERT(a, op, b) do { a; } while(0);
#define RESULT_TRUE(a) do { a; } while(0);
#endif /* OCR_ASSERT */


#endif /* __DEBUG_H__ */
