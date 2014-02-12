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

/**
 * @brief find hashtable entry for 'key'.
 */
static ocr_hashtable_entry* hashtableFindEntry(hashtable_t * hashtable, void * key, ocr_hashtable_entry** prev) {
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
  ocr_hashtable_entry ** table = pd->pdMalloc(pd, nbBuckets*sizeof(ocr_hashtable_entry*));
  u32 i;
  for (i=0; i < nbBuckets; i++) {
    table[i] = NULL;
  }
  hashtable->table = table;
  hashtable->nbBuckets = nbBuckets;
  hashtable->hashing = hashing;
}

/**
 * @brief get the value associated with a key
 */
void * hashtableGet(hashtable_t * hashtable, void * key) {
  ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key, NULL);
  return (entry == NULL) ? NULL_GUID : entry->value;
}

/**
 * @brief Attempt to insert the key if absent.
 * Return the current value associated with the key 
 * if present in the table, otherwise returns the value 
 * passed as parameter.
 */
void * hashtableTryPut(hashtable_t * hashtable, void * key, void * value) {
    // Compete with other potential put
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocr_hashtable_entry * newHead = NULL;
    bool tryPut = true;
    ocrPolicyDomain_t * pd = NULL;
    while(tryPut) {
      // Lookup for the key
      ocr_hashtable_entry * oldHead = hashtable->table[bucket];
      // Ensure oldHead is read before iterating
      //TODO not sure which hashtable members should be volatile
      hal_fence();
      ocr_hashtable_entry * entry = hashtableFindEntry(hashtable, key, NULL);
      if (entry == NULL) {
        // key is not there, try to CAS the head to insert it
        if (newHead == NULL) {
          getCurrentEnv(&pd, NULL, NULL, NULL);
          newHead = pd->pdMalloc(pd, sizeof(ocr_hashtable_entry));
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
          pd->pdFree(pd, newHead);
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
bool hashtablePut(hashtable_t * hashtable, void * key, void * value) {
    // Compete with other potential put
    u32 bucket = hashtable->hashing(key, hashtable->nbBuckets);
    ocrPolicyDomain_t * pd;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    ocr_hashtable_entry * newHead = pd->pdMalloc(pd, sizeof(ocr_hashtable_entry));
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
 * Returns true if entry has been found and removed.
 */
bool hashtableRemove(hashtable_t * hashtable, void * key) {
  //DIST-TODO implement concurrent removal
  return false;
}

/**
 * @brief Create a new hashtable instance that uses the specified hashing function.
 */
hashtable_t * newHashtable(ocrPolicyDomain_t * pd, u32 nbBuckets, hashFct hashing) {
    hashtable_t * hashtable = pd->pdMalloc(pd, sizeof(hashtable_t));
    hashtableInit(pd, hashtable, nbBuckets, hashing);
    return hashtable;
}

/**
 * @brief Destruct the hashtable and all its entries (do not deallocate keys and values pointers).
 */
void destructHashtable(hashtable_t * hashtable) {
  ocrPolicyDomain_t * pd;
  getCurrentEnv(&pd, NULL, NULL, NULL);
  // go over each bucket and deallocate entries
  u32 i = 0;
  while(i < hashtable->nbBuckets) {
    struct _ocr_hashtable_entry_struct * bucketHead = hashtable->table[i];
    while (bucketHead != NULL) {
      struct _ocr_hashtable_entry_struct * next = bucketHead->nxt;
      pd->pdFree(pd, bucketHead);
      bucketHead = next;
    }
    i++;
  }
  pd->pdFree(pd, hashtable);
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
