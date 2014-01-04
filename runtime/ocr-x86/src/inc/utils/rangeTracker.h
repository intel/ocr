/**
 * @brief Utilities to track the tagging of non overlapping ranges
 *
 *
 */

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef __RANGETRACKER_H__
#define __RANGETRACKER_H__

#include "ocr-runtime-types.h"
#include "ocr-types.h"

/*
 * Idea: the tree's key are the range "poles"
 * The value is either invalid or an index into tags
 * that describes the tag of the region starting at (inclusive)
 * and above.
 *
 * There is also an array of heads for each tag.
 *
 * Need to think of the merging of the different tag regions
 */

struct _ocrPolicyDomain_t;

/**
 * @brief A node for an AVL binary tree
 *
 * This is a self-balancing tree. See https://en.wikipedia.org/wiki/AVL_tree
 */
typedef struct _avlBinaryNode_t {
    u64 key; /**< Key used for comparison for the AVL */
    u64 value; /**< Value associated with the key */
    struct _avlBinaryNode_t *left, *right; /**< Left, right subtrees */
                                                     
    u32 height; /**< Height of this node, for rebalancing purposes */
} avlBinaryNode_t;

typedef struct _tagNode_t {
    avlBinaryNode_t *node; /**< Node pointed to */
    ocrMemoryTag_t tag; /**< Tag for this node */
    u32 nextTag; /**< Next index in the array that has the same tag */
    u32 prevTag; /**< Previous index in the array that has the same tag */
} tagNode_t;

typedef struct _tagHead_t {
    u32 headIdx; /**< Index of the first element of tag-list in the tag array (0 == invalid) */
    // TODO: See if useful to introduce finer-grain locking
    //volatile u32 lock; /**< Local lock on the tag-list */
} tagHead_t;

typedef struct _rangeTracker_t {
    struct _ocrPolicyDomain_t *pd; /**< Policy domain this range-tracker operates in */
    u64 minimum, maximum; /**< Minimum and maximum of this range */
    u32 maxSplits, nextTag;
    avlBinaryNode_t *rangeSplits; /**< Binary tree containing the "splits" in the range.
                                   * The "split" point is the key and the value is an
                                   * index into tags describing the tag of the current
                                   * "split" point and ABOVE
                                   */
    tagNode_t *tags; /**< Tags in this range */
    tagHead_t heads[MAX_TAG]; /**< Heads to the linked-lists stored in tags */
    
    // TODO: For the lock, would be better to move to a reader-writer lock
    volatile u32 lock; /**< Lock protecting range */
} rangeTracker_t;


/**
 * @brief Initialize the range tracker.
 *
 * This does not do any allocation for the range tracker (it must be pre-allocated
 * at the address passed in as dest
 *
 * @param[in] pd            Policy-domain inside which this tracker will be used
 * @param[in] dest          Pointer to this rangeTracker_t
 * @param[in] maxSplits     Maximum number of splits that will occur
 * @param[in] minRange      Smallest value for the range (inclusive)
 * @param[in] maxRange      Largest value for the range (exclusive)
 * @param[in] initTag       Initial tag for the entire range
 */
void initializeRange(struct _ocrPolicyDomain_t *pd, rangeTracker_t *dest,
                     u32 maxSplits, u64 minRange, u64 maxRange,
                     ocrMemoryTag_t initTag);

/**
 * @brief Destroys the range
 *
 * Does not de-allocate the range itself but will
 * de-allocate any sub-datastructures
 *
 * @param[in] self          Pointer to this regionTracker_t
 */
void destroyRange(rangeTracker_t *self);

/**
 * @brief Split the range at startAddr and tag [startAddr; startAddr + size[ with
 * tag. This will over-write any old tags in that range
 *
 * @param[in] range         Pointer to this regionTracker_t
 * @param[in] startAddr     Start-address for the new tag
 * @param[in] size          Size of the chunk of the range to tag with the new tag
 * @param[in] tag           New tag
 * @return 0 on success
 */
u8 splitRange(rangeTracker_t *range, u64 startAddr, u64 size, ocrMemoryTag_t tag);

/**
 * @brief Gets the tag at address addr
 *
 * This also optionally returns the
 * start of the region with the same tag and the end of it (ie: extent
 * of region around addr that has the same tag)
 *
 * @param[in] range         Pointer to this regionTracker_t
 * @param[in] addr          Address to get the tag of
 * @param[out] startRange   If non-NULL, will return the start of the
 *                          range around addr (inclusive)
 * @param[out] endRange     If non-NULL, will return the end of th
 *                          range around addr (exclusive)
 * @param[out] tag          Tag at addr
 * @return 0 on success
 */
u8 getTag(rangeTracker_t *range, u64 addr, u64 *startRange, u64 *endRange,
          ocrMemoryTag_t *tag);

/**
 * @brief "Iterator" for regions that have a particular tag
 *
 * @param[in] range         Pointer to this regionTracker_T
 * @param[in] tag           Tag to search for
 * @param[out] startRange   Start of the range (inclusive)
 * @param[out] endRange     End of the range (exclusive)
 * @param[in/out] iterate   Initialize to 0 and then keep passing
 *                          to subsequent calls
 * @return:
 *   - 0 on success
 *   - 1 if tag is not found or all of them have been iterated
 *   - 2 if the range doesn't contain tag at all
 *   - 3 if other error
 */
u8 getRegionWithTag(rangeTracker_t *range, ocrMemoryTag_t tag, u64 *startRange,
                    u64 *endRange, u64 *iterate);

#endif /* __RANGETRACKER_H__ */
