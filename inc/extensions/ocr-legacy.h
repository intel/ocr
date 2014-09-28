/**
 * @brief Top level OCR legacy support. Include this file if you
 * want to call OCR from sequential code for example
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_LEGACY_H__
#define __OCR_LEGACY_H__

#warning Using ocr-legacy.h may not be supported on all platforms
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup OCRLegacy OCR used from legacy code
 * @brief API to use OCR from legacy programming models
 *
 *
 * @{
 **/

/**
 * @brief OCR legacy mode - Waits for an output event to be satisfied
 *
 * @warning This call is meant to be called from sequential C code
 * and is *NOT* supported optimally on all implementations of OCR.
 * This call runs contrary to the 'non-blocking EDT'
 * philosophy so use with care
 *
 * @param outputEvent       Event to wait on. Must be a STICKY or IDEM event
 * @return A GUID to the data-block that was used to satisfy the event
 */
ocrGuid_t ocrWait(ocrGuid_t outputEvent);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __OCR_LEGACY_H__ */
