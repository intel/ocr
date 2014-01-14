/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_MAPPABLE_H_
#define OCR_MAPPABLE_H_

#include "ocr-types.h"


/**
 * @brief Mappables is the base class of OCR modules
 *
 * It currently does not contain anything but this may change in the future.
 * Mapping of different instances (comp-target to comp-platform for example)
 * is handled directly in the bringing up of the runtime when the INI file
 * is parsed.
 */
typedef struct _ocrMappable_t {
} ocrMappable_t;

#endif /* OCR_MAPPABLE_H_ */
