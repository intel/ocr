/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include "ocr-config.h"
#include "ocr-policy-domain.h"

struct _ocr_hashtable_entry_struct;

/**
 * @brief hash function to be used for key hashing.
 *
 * @param key         the key to hash
 * @param nbBuckets   number of buckets in the table
 */
typedef u32 (*hashFct)(void * key, u32 nbBuckets);

typedef struct _hashtable {
    u32 nbBuckets;
    struct _ocr_hashtable_entry_struct ** table;
    /** @brief hashing function to determine bucket. */
    hashFct hashing;
} hashtable_t;

void * hashtableConcGet(hashtable_t * hashtable, void * key);
bool hashtableConcPut(hashtable_t * hashtable, void * key, void * value);
void * hashtableConcTryPut(hashtable_t * hashtable, void * key, void * value);
bool hashtableConcRemove(hashtable_t * hashtable, void * key, void ** value);


void * hashtableNonConcGet(hashtable_t * hashtable, void * key);
bool hashtableNonConcPut(hashtable_t * hashtable, void * key, void * value);
void * hashtableNonConcTryPut(hashtable_t * hashtable, void * key, void * value);
bool hashtableNonConcRemove(hashtable_t * hashtable, void * key, void ** value);

hashtable_t * newHashtable(ocrPolicyDomain_t * pd, u32 nbBuckets, hashFct hashing);
void destructHashtable(hashtable_t * hashtable);


//
// Exposed hashtable implementations
//

/*
 * @brief A hashtable implementation relying on a simple nbBuckets' modulo for hashing
 */
hashtable_t * newHashtableModulo(ocrPolicyDomain_t * pd, u32 nbBuckets);

#endif /* HASHTABLE_H_ */
