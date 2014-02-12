/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __GUID_ALL_H__
#define __GUID_ALL_H__

#include "debug.h"
#include "ocr-config.h"
#include "ocr-types.h"
#include "utils/ocr-utils.h"

typedef enum _guidType_t {
    guidPtr_id,
    guidCountedMap_id,
    guidMax_id
} guidType_t;

const char * guid_types[] __attribute__ ((weak)) = {
    "PTR",
    "COUNTED_MAP",
    NULL
};

// Ptr GUID provider
#include "guid/ptr/ptr-guid.h"
// Counted GUID provider backed my map
#include "guid/counted/counted-map-guid.h"

// Add other GUID providers if needed
static inline ocrGuidProviderFactory_t *newGuidProviderFactory(guidType_t type, ocrParamList_t *typeArg) {
    switch(type) {
#ifdef ENABLE_GUID_PTR
    case guidPtr_id:
        return newGuidProviderFactoryPtr(typeArg, (u32)type);
#endif
#ifdef ENABLE_GUID_COUNTED_MAP
    case guidCountedMap_id:
        return newGuidProviderFactoryCountedMap(typeArg, (u32)type);
#endif
    default:
        ASSERT(0);
    }
    return NULL;
}
#endif /* __GUID_ALL_H__ */
