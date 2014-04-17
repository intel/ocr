/**
 * @brief OCR compute platforms
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-comp-platform.h"
#include "ocr-policy-domain.h"

ocrGuid_t (*getCurrentEDT)() = NULL;
void (*setCurrentEDT)(ocrGuid_t) = NULL;
