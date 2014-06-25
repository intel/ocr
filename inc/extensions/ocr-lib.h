/**
 * @brief Top level OCR library file. Include this file in any
 * program making use of OCR. You do not need to include any other
 * files.
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_LIB_H__
#define __OCR_LIB_H__
#ifdef __cplusplus
extern "C" {
#endif
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
#ifdef __cplusplus
}
#endif

#endif /* __OCR_LIB_H__ */
