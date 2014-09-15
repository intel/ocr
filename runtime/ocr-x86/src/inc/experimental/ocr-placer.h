/**
 * @brief Temporary "placer" infrastructure (converting affinities to locations)
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifndef __OCR_PLACER_H__
#define __OCR_PLACER_H__

#include "ocr-types.h"
#include "ocr-runtime-types.h"

typedef struct _ocrPlacer_t {
} ocrPlacer_t;

// Affinity data-structure, tailored to location placer.
// work in progress, likely to be changed
typedef struct _ocrAffinity_t {
    ocrLocation_t place;
} ocrAffinity_t;

typedef struct _ocrLocationPlacer_t {
    ocrPlacer_t base;
    u32 lock; /**< Lock for updating edtLastPlacementIndex information */
    u64 edtLastPlacementIndex; /**< Index of the last guid returned for an edt */
    u64 pdLocAffinitiesSize; /**< Count of available locations */
    u32 current;
    ocrGuid_t * pdLocAffinities;
} ocrLocationPlacer_t;

struct _ocrPolicyDomain_t;
struct _ocrPolicyMsg_t;

ocrLocation_t affinityToLocation(ocrGuid_t affinityGuid);

ocrPlacer_t * createLocationPlacer(struct _ocrPolicyDomain_t * pd);
u8 suggestLocationPlacement(struct _ocrPolicyDomain_t *pd, ocrLocation_t curLoc, ocrLocationPlacer_t * placer, struct _ocrPolicyMsg_t * msg);

#endif /* __OCR_PLACER_H__ */
