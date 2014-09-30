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
 * @ingroup OCRExt
 * @{
 */
/**
 * @defgroup OCRExtLegacy OCR used from legacy code
 * @brief API to use OCR from legacy programming models
 *
 *
 * @{
 **/

/**
 * @brief Waits on the satisfaction of an event
 *
 * @warning The event waited on must be a persistent event (in other
 * words, not a LATCH or ONCE event).
 *
 * @warning This call may be slow and inefficient. It is meant
 * only to call OCR from sequential code and should be avoided in a
 * pure OCR program
 *
 * @param[in] outputEvent   GUID of the event to wait on
 *
 * @return The GUID of the data block on the post-slot of the
 * waited-on event
 */
ocrGuid_t ocrWait(ocrGuid_t outputEvent);

/**
 * @}
 * @}
 */
#ifdef __cplusplus
}
#endif

#endif /* __OCR_LEGACY_H__ */
