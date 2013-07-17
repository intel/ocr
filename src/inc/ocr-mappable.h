/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_MAPPABLE_H_
#define OCR_MAPPABLE_H_

#include "ocr-types.h"

typedef enum _ocrMappableKind {
    OCR_COMP_PLATFORM = 0,
    OCR_COMP_TARGET = 1,
    OCR_WORKER = 2,
    OCR_WORKER_FACTORY = 3,
    OCR_EDT_FACTORY = 4,
    OCR_MEM_PLATFORM = 5,
    OCR_MEM_TARGET = 6,
    OCR_ALLOCATOR = 7,
    OCR_ALLOCATOR_FACTORY = 8,
    OCR_DB_FACTORY = 9,
    OCR_WORKPILE = 10,
    OCR_WORKPILE_FACTORY = 11,
    OCR_SCHEDULER = 12,
    OCR_SCHEDULER_FACTORY = 13,
    OCR_GUIDPROVIDER = 14,
    OCR_GUIDPROVIDER_FACTORY = 15,
    OCR_EVENT_FACTORY = 16,
    OCR_POLICY = 17
} ocrMappableKind;

typedef enum _ocrMappingKind {
    ONE_TO_ONE_MAPPING  = 0,
    MANY_TO_ONE_MAPPING = 1,
    ONE_TO_MANY_MAPPING = 2
} ocrMappingKind;

typedef struct _ocrModuleMapping_t {
    ocrMappingKind kind;
    ocrMappableKind from;
    ocrMappableKind to;
} ocrModuleMapping_t;

struct _ocrMappable_t;

typedef void (*ocrMapFct_t) (struct _ocrMappable_t * self, ocrMappableKind kind,
                             u64 instanceCount, struct _ocrMappable_t ** instances);

typedef struct _ocrMappable_t {
    ocrMapFct_t mapFct;
} ocrMappable_t;


#endif /* OCR_MAPPABLE_H_ */
