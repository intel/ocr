/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __GUID_ALL_H__
#define __GUID_ALL_H__

#include "debug.h"
#include "ocr-types.h"
#include "ocr-utils.h"

typedef enum _guidType_t {
    guidPtr_id,
    guidMax_id
} guidType_t;

const char * guid_types[] __attribute__ ((weak)) = {
    "PTR",
    NULL
};

// Ptr GUID provider
#include "guid/ptr/ptr-guid.h"

// Add other GUID providers if needed
static inline ocrGuidProviderFactory_t *newGuidProviderFactory(guidType_t  type, ocrParamList_t *typeArg) {
    switch(type) {
    case guidPtr_id:
        return newGuidProviderFactoryPtr(typeArg);
    default:
        ASSERT(0);
    }
    return NULL;
}
#endif /* __GUID_ALL_H__ */
