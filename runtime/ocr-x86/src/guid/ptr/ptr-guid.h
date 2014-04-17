/**
 * @brief Very simple GUID implementation that basically considers
 * pointers to be GUIDs
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_GUIDPROVIDER_PTR_H__
#define __OCR_GUIDPROVIDER_PTR_H__

#include "ocr-types.h"
#include "ocr-guid.h"

/**
 * @brief Trivial GUID provider
 * @warning This will always restore OCR_GUID_NONE for the type.
 * In other words, it never stores the type
 */
typedef struct {
    ocrGuidProvider_t base;
} ocrGuidProviderPtr_t;

typedef struct {
    ocrGuidProviderFactory_t base;
} ocrGuidProviderFactoryPtr_t;

ocrGuidProviderFactory_t* newGuidProviderFactoryPtr(ocrParamList_t *typeArg);

#define __GUID_END_MARKER__
#include "ocr-guid-end.h"
#undef __GUID_END_MARKER__

#endif /* __OCR_GUIDPROVIDER_PTR_H__ */
