/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"

#include "ocr-hal.h"
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "utils/hashtable.h"

/* Hashtable entries */
typedef struct _ocr_hashtable_entry_struct {
    void * key;
    void * value;
    struct _ocr_hashtable_entry_struct * nxt; /* the next bucket in the hashtable */
} ocr_hashtable_entry;

/******************************************************/
/* CONCURRENT BUCKET LOCKED HASHTABLE                 */
/******************************************************/

typedef struct _hashtableBucketLocked_t {
    hashtable_t base;
    u32 * bucketLock;
} hashtableBucketLocked_t;

/******************************************************/
/* HASHTABLE COMMON FUNCTIONS                         */
/******************************************************/

static ocr_hashtable_entry* hashtableFindEntryAndPrev(hashtable_t * hashtable, void * key, ocr_hashtable_entry** prev) {
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocr_hashtable_entry * current = hashtable->table[bucket];
    *prev = current;
    while(current != NULL) {
        if (current->key == key) {
            return current;
        }
        *prev = current;
        current = current->nxt;
    }
    *prev = NULL;
    return NULL;
}

/**
 * @brief find hashtable entry for 'key'.
 */
static ocr_hashtable_entry* hashtableFindEntry(hashtable_t * hashtable, void * key) {
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocr_hashtable_entry * current = hashtable->table[bucket];
    while(current != NULL) {
        if (current->key == key) {
            return current;
        }
        current = current->nxt;
    }
    return NULL;
}

/**
 * @brief Initialize the hashtable.
 */
static void hashtableInit(ocrPolicyDomain_t * pd, hashtable_t * hashtable, u32 nbBuckets, hashFct hashing) {
    hashtable->pd = pd;
    ocr_hashtable_entry ** table = pd->fcts.pdMalloc(pd, nbBuckets*sizeof(ocr_hashtable_entry*));
    u32 i;
    for (i=0; i < nbBuckets; i++) {
        table[i] = NULL;
    }
    hashtable->table = table;
    hashtable->nbBuckets = nbBuckets;
    hashtable->hashing = hashing;
}

/**
 * @brief Create a new hashtable instance that uses the specified hashing function.
 */
hashtable_t * newHashtable(ocrPolicyDomain_t * pd, u32 nbBuckets, hashFct hashing) {
    hashtable_t * hashtable = pd->fcts.pdMalloc(pd, sizeof(hashtable_t));
    hashtableInit(pd, hashtable, nbBuckets, hashing);
    return hashtable;
}

/**
 * @brief Destruct the hashtable and all its entries (do not deallocate keys and values pointers).
 */
void destructHashtable(hashtable_t * hashtable) {
    ocrPolicyDomain_t * pd = hashtable->pd;
    // go over each bucket and deallocate entries
    u32 i = 0;
    while(i < hashtable->nbBuckets) {
        struct _ocr_hashtable_entry_struct * bucketHead = hashtable->table[i];
        while (bucketHead != NULL) {
            struct _ocr_hashtable_entry_struct * next = bucketHead->nxt;
            pd->fcts.pdFree(pd, bucketHead);
            bucketHead = next;
        }
        i++;
    }
    pd->fcts.pdFree(pd, hashtable->table);
    pd->fcts.pdFree(pd, hashtable);
}

/**
 * @brief Create a new hashtable bucket locked instance that uses the specified hashing function.
 */
hashtable_t * newHashtableBucketLocked(ocrPolicyDomain_t * pd, u32 nbBuckets, hashFct hashing) {
    hashtable_t * hashtable = pd->fcts.pdMalloc(pd, sizeof(hashtableBucketLocked_t));
    hashtableInit(pd, hashtable, nbBuckets, hashing);
    hashtableBucketLocked_t * rhashtable = (hashtableBucketLocked_t *) hashtable;
    u32 i;
    u32 * bucketLock = pd->fcts.pdMalloc(pd, nbBuckets*sizeof(u32));
    for (i=0; i < nbBuckets; i++) {
        bucketLock[i] = 0;
    }
    rhashtable->bucketLock = bucketLock;
    return hashtable;
}

/**
 * @brief Destruct the hashtable and all its entries (do not deallocate keys and values pointers).
 */
void destructHashtableBucketLocked(hashtable_t * hashtable) {
    ocrPolicyDomain_t * pd = hashtable->pd;
    hashtableBucketLocked_t * rhashtable = (hashtableBucketLocked_t *) hashtable;
    pd->fcts.pdFree(pd, rhashtable->bucketLock);
    destructHashtable(hashtable);
}

/******************************************************/
/* NON-CONCURRENT HASHTABLE                           */
/******************************************************/

/**
 * @brief get the value associated with a key
 */
void * hashtableNonConcGet(hashtable_t * hashtable, void * key) {
    ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key);
    return (entry == NULL) ? NULL_GUID : entry->value;
}

/**
 * @brief Attempt to insert the key if absent.
 * Return the current value associated with the key
 * if present in the table, otherwise returns the value
 * passed as parameter.
 */
void * hashtableNonConcTryPut(hashtable_t * hashtable, void * key, void * value) {
    ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key);
    if (entry == NULL) {
        hashtableNonConcPut(hashtable, key, value);
        return value;
    } else {
        return entry->value;
    }
}

/**
 * @brief Put a key associated with a given value in the map.
 */
bool hashtableNonConcPut(hashtable_t * hashtable, void * key, void * value) {
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocrPolicyDomain_t * pd = hashtable->pd;
    ocr_hashtable_entry * newHead = pd->fcts.pdMalloc(pd, sizeof(ocr_hashtable_entry));
    newHead->key = key;
    newHead->value = value;
    ocr_hashtable_entry * oldHead = hashtable->table[bucket];
    newHead->nxt = oldHead;
    hashtable->table[bucket] = newHead;
    return true;
}

/**
 * @brief Removes a key from the table.
 * If 'value' is not NULL, fill-in the pointer with the entry's value associated with 'key'.
 * If the hashtable implementation allows for NULL values entries, check the returned boolean.
 * Returns true if entry has been found and removed.
 */
bool hashtableNonConcRemove(hashtable_t * hashtable, void * key, void ** value) {
    ocr_hashtable_entry * prev;
    ocr_hashtable_entry * entry = hashtableFindEntryAndPrev(hashtable, key, &prev);
    if (entry != NULL) {
        ASSERT(prev != NULL);
        ASSERT(key == entry->key);
        if (entry == prev) {
            // entry is bucket's head
            u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
            hashtable->table[bucket] = entry->nxt;
        } else {
            prev->nxt = entry->nxt;
        }
        if (value != NULL) {
            *value = entry->value;
        }
        hashtable->pd->fcts.pdFree(hashtable->pd, entry);
        return true;
    }
    return false;
}


/******************************************************/
/* CONCURRENT HASHTABLE FUNCTIONS                     */
/******************************************************/

/**
 * @brief get the value associated with a key
 */
void * hashtableConcGet(hashtable_t * hashtable, void * key) {
    ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key);
    return (entry == NULL) ? NULL_GUID : entry->value;
}

/**
 * @brief Attempt to insert the key if absent.
 * Return the current value associated with the key
 * if present in the table, otherwise returns the value
 * passed as parameter.
 */
void * hashtableConcTryPut(hashtable_t * hashtable, void * key, void * value) {
    // Compete with other potential put
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocr_hashtable_entry * newHead = NULL;
    bool tryPut = true;
    ocrPolicyDomain_t * pd = hashtable->pd;
    while(tryPut) {
        // Lookup for the key
        ocr_hashtable_entry * oldHead = hashtable->table[bucket];
        // Ensure oldHead is read before iterating
        //TODO not sure which hashtable members should be volatile
        hal_fence();
        ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key);
        if (entry == NULL) {
            // key is not there, try to CAS the head to insert it
            if (newHead == NULL) {
                newHead = pd->fcts.pdMalloc(pd, sizeof(ocr_hashtable_entry));
                newHead->key = key;
                newHead->value = value;
            }
            newHead->nxt = oldHead;
            // if old value read is different from the old value read by the cmpswap we failed
            tryPut = (hal_cmpswap64((u64*)&hashtable->table[bucket], (u64)oldHead, (u64)newHead) != (u64)oldHead);
            // If failed, go for another round of check, there's been an insert but we don't
            // know if it's for that key or another one which hash matches the bucket
        } else {
            if (newHead != NULL) {
                ASSERT(pd != NULL);
                // insertion failed because the key had been inserted by a concurrent thread
                pd->fcts.pdFree(pd, newHead);
            }
            // key already present, return the value
            return entry->value;
        }
    }
    // could successfully insert the (key,value) pair
    return value;
}

/**
 * @brief Put a key associated with a given value in the map.
 * The put always occurs.
 */
bool hashtableConcPut(hashtable_t * hashtable, void * key, void * value) {
    // Compete with other potential put
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocrPolicyDomain_t * pd = hashtable->pd;
    ocr_hashtable_entry * newHead = pd->fcts.pdMalloc(pd, sizeof(ocr_hashtable_entry));
    newHead->key = key;
    newHead->value = value;
    bool success;
    do {
        ocr_hashtable_entry * oldHead = hashtable->table[bucket];
        newHead->nxt = oldHead;
        success = (hal_cmpswap64((u64*)&hashtable->table[bucket], (u64)oldHead, (u64)newHead) == (u64)oldHead);
    } while (!success);
    return success;
}

/**
 * @brief Removes a key from the table.
 * If 'value' is not NULL, fill-in the pointer with the entry's value associated with 'key'.
 * If the hashtable implementation allows for NULL values entries, check the returned boolean.
 * Returns true if entry has been found and removed.
 */
bool hashtableConcRemove(hashtable_t * hashtable, void * key, void ** value) {
    //TODO implement concurrent removal
    ASSERT(false);
    return false;
}


/******************************************************/
/* CONCURRENT BUCKET LOCKED HASHTABLE                 */
/******************************************************/

void * hashtableConcBucketLockedGet(hashtable_t * hashtable, void * key) {
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    hashtableBucketLocked_t * rhashtable = (hashtableBucketLocked_t *) hashtable;
    hal_lock32(&(rhashtable->bucketLock[bucket]));
    ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key);
    hal_unlock32(&(rhashtable->bucketLock[bucket]));
    return (entry == NULL) ? NULL_GUID : entry->value;
}

bool hashtableConcBucketLockedPut(hashtable_t * hashtable, void * key, void * value) {
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    hashtableBucketLocked_t * rhashtable = (hashtableBucketLocked_t *) hashtable;
    ocr_hashtable_entry * newHead = hashtable->pd->fcts.pdMalloc(hashtable->pd, sizeof(ocr_hashtable_entry));
    newHead->key = key;
    newHead->value = value;
    hal_lock32(&(rhashtable->bucketLock[bucket]));
    ocr_hashtable_entry * oldHead = hashtable->table[bucket];
    newHead->nxt = oldHead;
    hashtable->table[bucket] = newHead;
    hal_unlock32(&(rhashtable->bucketLock[bucket]));
    return true;
}

void * hashtableConcBucketLockedTryPut(hashtable_t * hashtable, void * key, void * value) {
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    hashtableBucketLocked_t * rhashtable = (hashtableBucketLocked_t *) hashtable;
    hal_lock32(&(rhashtable->bucketLock[bucket]));
    ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key);
    if (entry == NULL) {
        hashtableNonConcPut(hashtable, key, value); // we already own the lock
        hal_unlock32(&(rhashtable->bucketLock[bucket]));
        return value;
    } else {
        hal_unlock32(&(rhashtable->bucketLock[bucket]));
        return entry->value;
    }
}

bool hashtableConcBucketLockedRemove(hashtable_t * hashtable, void * key, void ** value) {
    hashtableBucketLocked_t * rhashtable = (hashtableBucketLocked_t *) hashtable;
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    hal_lock32(&(rhashtable->bucketLock[bucket]));
    bool res = hashtableNonConcRemove(hashtable, key, value);
    hal_unlock32(&(rhashtable->bucketLock[bucket]));
    return res;
}


//
// Variants of the generic hashtable through hashing function specialization
//

/*
 * This implementation is meant to be used with monotonically generated guids.
 * Modulo more or less ensure buckets' load is balanced.
 */
static u32 hashModulo(void * ptr, u32 nbBuckets) {
    return (((unsigned long) ptr) % nbBuckets);
}

hashtable_t * newHashtableModulo(ocrPolicyDomain_t * pd, u32 nbBuckets) {
    return newHashtable(pd, nbBuckets, hashModulo);
}

hashtable_t * newHashtableBucketLockedModulo(ocrPolicyDomain_t * pd, u32 nbBuckets) {
    return newHashtableBucketLocked(pd, nbBuckets, hashModulo);
}
