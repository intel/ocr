/**
 * @brief Tuning language API for OCR. This is an experimental feature
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __OCR_AFFINITY_H__
#define __OCR_AFFINITY_H__

#warning Using ocr-affinity.h may not be supported on all platforms

#ifdef __cplusplus
extern "C" {
#endif
// places include RAM, SP, or NUMA if available

#include "ocr-types.h"

//
// User-level API (experimental - may be deprecated anytime)
//

/**
 * @defgroup OCRExt OCR extensions/experimental APIs
 *
 * @brief These APIs are experimental or extensions that
 * are not necessarily supported on all platforms. They are
 * subject to change
 *
 * @{
 */
/**
 * @defgroup OCRExtAffinity Affinity extension
 *
 * @brief This extension is primarily used for the
 * distributed implementation of OCR to specify better
 * placement for EDTs and data blocks
 *
 * @{
 */

/**
 * @brief Types of affinities
 *
 * You can query for the affinities corresponding to
 * policy domains as well as for your own affinities
 */
typedef enum {
    AFFINITY_CURRENT, /**< Affinities of the current EDT */
    AFFINITY_PD, /**< Affinities of the policy domains. You can then
                  * affinitize EDTs and data blocks to these affinities
                  * in the creation calls */
    AFFINITY_PD_MASTER /**< Runtime reserved (do not use) */
} ocrAffinityKind;

/**
 * @brief Returns a count of affinity GUIDs of a particular kind.
 *
 * @param[in] kind      The affinity kind to query for. See #ocrAffinityKind
 * @param[out] count    Count of affinity GUIDs of that kind in the system
 * @return a status code
 *      - 0: successful
 */
u8 ocrAffinityCount(ocrAffinityKind kind, u64 * count);

/**
 * @brief Gets the affinity GUIDs of a particular kind. The 'affinities' array
 * must have been previously allocated and big enough to contain 'count'
 * GUIDs.
 *
 * @param[in] kind             The affinity kind to query for. See #ocrAffinityKind
 * @param[in,out] count        As input, requested number of elements; as output
 *                             the actual number of elements returned
 * @param[out] affinities      Affinity GUID array
 * @return a status code
 *      - 0: successful
 */
u8 ocrAffinityGet(ocrAffinityKind kind, u64 * count, ocrGuid_t * affinities);

/**
 * @brief Returns an affinity the currently executing EDT is affinitized to
 *
 * An EDT may have multiple affinities. The programmer should rely on
 * ocrAffinityCount() and ocrAffinityGet() to query all affinities.
 *
 * @param[out] affinity    One affinity GUID for the currently executing EDT
 * @return a status code
 *      - 0: successful
 */
u8 ocrAffinityGetCurrent(ocrGuid_t * affinity);

#ifdef __cplusplus
}
#endif
/**
 * @}
 * @}
 */
#endif /* __OCR_AFFINITY_H__ */

