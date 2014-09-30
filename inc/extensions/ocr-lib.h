/**
 * @brief OCR APIs to use OCR in a library mode
 *
 * OCR is designed to be used in a standalone manner (using
 * mainEdt) but can also be called from a traditional main function.
 * These functions enable the programmer to startup and shutdown the
 * OCR runtime
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_LIB_H__
#define __OCR_LIB_H__

#warning Using ocr-lib.h may not be supported on all platforms
#ifdef __cplusplus
extern "C" {
#endif
#include "ocr-db.h"

/**
 * @ingroup OCRExt
 * @{
 */
/**
 * @defgroup OCRExtLib OCR used as a library extension
 * @brief Describes the OCR as library API
 *
 * When OCR is used as a library, the user need to
 * explicitly bring up and tear down the runtime using
 * the APIs provided here.
 *
 * @{
 **/

/**
 * @brief Data-structure containing the configuration
 * parameters for the runtime
 *
 * The contents of this struct can be filled in by calling
 * ocrParseArgs() or by setting them manually. The former
 * method is strongly recommended.
 *
 */
typedef struct _ocrConfig_t {
    int userArgc; /**< Application argc (after having stripped the OCR arguments) */
    char ** userArgv; /**< Application argv (after having stripped the OCR arguments) */
    const char * iniFile; /**< INI configuration file for the runtime */
} ocrConfig_t;

/**
 * @brief Bring up the OCR runtime
 *
 * This function needs to be called to bring up the runtime. It
 * should be called once for each runtime that needs to be brought
 * up.
 *
 * @param[in] ocrConfig  Configuration parameters to bring up the
 *                       runtime
 */
void ocrInit(ocrConfig_t * ocrConfig);

/**
 * @brief Parses the arguments passed to main and extracts the
 * relevant information to initialize OCR
 *
 * This should be called prior to ocrInit() to populate the
 * #ocrConfig_t variable needed by ocrInit().
 *
 * @param[in] argc           The number of elements in argv
 * @param[in] argv           Array of char * argumetns.
 * @param[in,out] ocrConfig  Pointer to an ocrConfig ocrParseArgs will populate. ocrConfig
 *                           needs to have already been allocated
 */
void ocrParseArgs(int argc, const char* argv[], ocrConfig_t * ocrConfig);

/**
 * @brief Prepares to tear down the OCR runtime
 *
 * This call prepares the runtime to be torn-down. This call
 * will only return after the OCR program completes (ie: after
 * the program calls ocrShutdown()).
 *
 * @return the status code of the OCR program:
 *      - 0: clean shutdown, no errors
 *      - non-zero: user provided error code to ocrAbort()
 */
u8 ocrFinalize();

/**
 * @}
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __OCR_LIB_H__ */
