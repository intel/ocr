/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "extensions/ocr-affinity.h"
#include "ocr-policy-domain.h"
#include "experimental/ocr-placer.h"
#include "ocr-errors.h"


//Part of policy-domain debug messages
#define DEBUG_TYPE POLICY

//
// Affinity group API
//

// These allow to handle the simple case where the caller request a number
// of affinity guids and does the mapping. i.e. assign some computation
// and data to each guid.

u8 ocrAffinityCount(ocrAffinityKind kind, u64 * count) {
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    // If no placer, we know nothing so just say that we are the only one
    if(pd->placer == NULL) {
        *count = 1;
        return 0;
    }

    if (kind == AFFINITY_PD) {
        //TODO this is assuming each PD knows about every other PDs
        //Need to revisit that when we have a better idea of what affinities are
        *count = (pd->neighborCount + 1);
    } else if ((kind == AFFINITY_PD_MASTER) || (kind == AFFINITY_CURRENT)) {
        // Change this implementation if 'AFFINITY_CURRENT' cardinality can be > 1
        *count = 1;
    } else {
        ASSERT(false && "Unknown affinity kind");
    }
    return 0;
}

//TODO This call returns affinities with identical mapping across PDs.
u8 ocrAffinityGet(ocrAffinityKind kind, u64 * count, ocrGuid_t * affinities) {
    ocrPolicyDomain_t * pd = NULL;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    ocrLocationPlacer_t * placer = ((ocrLocationPlacer_t*)pd->placer);
    if(placer == NULL) {
        ASSERT(*count > 0);
        *count = 1;
        affinities[0] = NULL_GUID;
        return 0;
    }

    if (kind == AFFINITY_PD) {
        ASSERT(*count <= (pd->neighborCount + 1));
        u64 i = 0;
        while(i < *count) {
            affinities[i] = placer->pdLocAffinities[i];
            i++;
        }
    } else if (kind == AFFINITY_PD_MASTER) {
        //TODO This should likely come from the INI file
        affinities[0] = placer->pdLocAffinities[0];
    } else if (kind == AFFINITY_CURRENT) {
        affinities[0] = placer->pdLocAffinities[placer->current];
    } else {
        ASSERT(false && "Unknown affinity kind");
    }
    return 0;
}

u8 ocrAffinityGetCurrent(ocrGuid_t * affinity) {
    u64 count = 1;
    return ocrAffinityGet(AFFINITY_CURRENT, &count, affinity);
}
