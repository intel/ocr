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


void initializeRange(rangeTracker_t *dest, u32 maxSplits, u64 minRange, u64 maxRange, ocrMemoryTag_t initTag);

void destroyRange(rangeTracker_t *self);

u8 splitRange(rangeTracker_t *range, u64 startAddr, u64 size, ocrMemoryTag_t tag);

u8 getTag(rangeTracker_t *range, u64 addr, u64 *startRange, u64 *endRange, ocrMemoryTag_t *tag);

u8 getRegionWithTag(rangeTracker_t *range, ocrMemoryTag_t tag, u64 *startRange, u64 *endRange, u64 *iterate);
#endif /* __RANGETRACKER_H__ */
