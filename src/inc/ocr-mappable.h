/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef OCR_MAPPABLE_H_
#define OCR_MAPPABLE_H_

#include "ocr-types.h"

/**
 * @brief Kinds of mappable
 */
typedef enum _ocrMappableKind {
    OCR_COMP_PLATFORM = 0,
    OCR_COMP_TARGET = 1,
    OCR_WORKER = 2,
    OCR_MEM_PLATFORM = 3,
    OCR_MEM_TARGET = 4,
    OCR_ALLOCATOR = 5,
    OCR_WORKPILE = 6,
    OCR_SCHEDULER = 7,
    OCR_POLICY = 8
} ocrMappableKind;

/**
 * @brief Describes the direction of a mapping
 */
typedef enum _ocrMappingKind {
    ONE_TO_ONE_MAPPING  = 0,
    MANY_TO_ONE_MAPPING = 1,
    ONE_TO_MANY_MAPPING = 2
} ocrMappingKind;

/**
 * @brief Binding description from a mappable to
 * another mappable. 'kind' describes the the
 * direction of the mapping
 */
typedef struct _ocrModuleMapping_t {
    ocrMappingKind kind; /**<  kind of binding */
    ocrMappableKind from; /**<  the mappable to be mapped */
    ocrMappableKind to; /**<  the mappable being bound to */
} ocrModuleMapping_t;

struct _ocrMappable_t;

/**
 * @brief Function pointer to bind mappables
 * \param[in] self              Pointer to this mappable
 * \param[in] kind              The kind of this mappable
 * \param[in] instanceCount     Number of instances
 * \param[in] instances         Mappable instances to be mapped
 */
typedef void (*ocrMapFct_t) (struct _ocrMappable_t * self, ocrMappableKind kind,
                             u64 instanceCount, struct _ocrMappable_t ** instances);

/**
 * @brief Mappables is the base class of OCR modules
 */
typedef struct _ocrMappable_t {
    ocrMapFct_t mapFct; /**< Mappable function pointer to be used to map this mappable instance to some other mappables */
} ocrMappable_t;

#endif /* OCR_MAPPABLE_H_ */
