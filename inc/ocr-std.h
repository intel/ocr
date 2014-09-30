/**
 * @brief Limited "standard" API for OCR
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_STD_H__
#define __OCR_STD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "ocr-types.h"


/**
 * @defgroup OCRStandrdAPI Limited "standard" API for OCR
 * @brief Describes the limited "standard" API for OCR
 *
 * This file provides APIs for very limited standard functions
 * that replicate certain libc functions.
 *
 * @note This may be extended in the future.
 *
 * @{
 **/


/**
 * @brief Console output
 *
 * This maps to the typical C-like printf() functionality.
 *
 * @param[in] fmt       Format specifier, as per printf(). Not
 *                      all modifiers are supported. Standard ones
 *                      like %d, %f, %s, %x, %p work properly
 * @param[in] ...       Arguments, as per printf().
 *
 * @return number of characters printed, as per printf().
 *
 **/
extern u32 PRINTF(const char * fmt, ...);

/**
 * @brief Platform independent 'assert' functionality
 *
 * This will cause the program to abort and return an
 * assertion failure.
 * This function should be called using the #ASSERT macro
 *
 * @param[in] val       If non-zero, will cause the assertion failure
 * @param[in] file      File in which the assertion failure occured
 * @param[in] line      Line at which the assertion failure occured
 */
extern void _ocrAssert(bool val, const char* file, u32 line);

#ifdef OCR_ASSERT
/**
 * @brief ASSERT macro to replace the assert functionality
 *
 * @param[in] a  Condition for the assert
 */
#define ASSERT(a) do { _ocrAssert((bool)((a) != 0), __FILE__, __LINE__); } while(0);
#else
/**
 * @brief ASSERT macro to replace the assert functionality
 *
 * @param[in] a  Condition for the assert
 */
#define ASSERT(a)
#endif

/**
 * @brief Primitive method to print FAILURE or PASSED
 * messages
 *
 * This macro is used to simply verify if the execution
 * of the program is correct and prints out either a
 * message starting with PASSED if successful or FAILURE
 * if not
 *
 * @param[in] cond    If true, will print a PASSED message.
 * @param[in] format  Message to print (see #PRINTF)
 * @param[in] ...     Arguments for message
 */
#define VERIFY(cond, format, ...)                                       \
    do {                                                                \
        if(!(cond)) {                                                   \
            PRINTF("FAILURE @ '%s:%d' " format, __FILE__, __LINE__, ## __VA_ARGS__); \
        } else {                                                        \
            PRINTF("PASSED @ '%s:%d' " format, __FILE__, __LINE__, ## __VA_ARGS__); \
        }                                                               \
    } while(0);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __OCR_STD_H__ */


