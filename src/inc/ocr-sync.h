/**
 * @brief OCR internal interface for locks and other internal
 * synchronization mechanisms
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */



#ifndef __OCR_SYNC_H__
#define __OCR_SYNC_H__

#include "ocr-types.h"
#include "ocr-utils.h"

/****************************************************/
/* PARAMETER LISTS                                  */
/****************************************************/
typedef struct {
    ocrParamList_t base;
} paramListLockFact_t;

typedef struct {
    ocrParamList_t base;
} paramListLockInst_t;

typedef struct {
    ocrParamList_t base;
} paramListAtomic64Fact_t;

typedef struct {
    ocrParamList_t base;
} paramListAtomic64Inst_t;

typedef struct {
    ocrParamList_t base;
} paramListQueueFact_t;

typedef struct {
    ocrParamList_t base;
} paramListQueueInst_t;


/****************************************************/
/* OCR LOCK                                         */
/****************************************************/

struct _ocrLock_t;

typedef struct _ocrLockFcts_t {
    /**
     * @brief Destructor equivalent
     *
     * Cleans up the lock and frees the metadata associated with it.
     * This is the mirror call the task factory's instantiate call
     *
     * @param self          Pointer to this lock
     */
    void (*destruct)(struct _ocrLock_t* self);

    /**
     * @brief Grab the lock
     *
     * This calls blocks until the lock is grabbed
     *
     * @param self          Pointer to this lock
     */
    void (*lock)(struct _ocrLock_t* self);

    /**
     * @brief Release the lock
     *
     * Note that the implementation determines
     * whether or not only the entity that had
     * locked the lock can unlock it. This call
     * is non-blocking
     *
     * @param self          Pointer to this lock
     */
    void (*unlock)(struct _ocrLock_t* self);

    /**
     * @brief Tries to grab the lock
     *
     * Returns 1 on success or 0 on failure.
     * Non-blocking call in both cases
     *
     * @param self      This lock
     * @return 1 if the lock was successfully grabbed, 0 if not
     */
    u8 (*trylock)(struct _ocrLock_t* self);
} ocrLockFcts_t;

/**
 * @brief A model for a very simple lock that
 * can be used in the runtime
 *
 * Locks are not exposed to the user as the EDT
 * model does not require them
 */
typedef struct _ocrLock_t {
    ocrLockFcts_t *fctPtrs;
} ocrLock_t;

/****************************************************/
/* OCR LOCK FACTORY                                 */
/****************************************************/

/**
 * @brief Factory for locks
 */
typedef struct _ocrLockFactory_t {
    /**
     * @brief Destroy the factory freeing up any
     * memory associated with it
     *
     * @param self          This factory

     * @warning This call will not necessarily destroy
     * all locks created with the factory
     */
    void (*destruct)(struct _ocrLockFactory_t *self);


    ocrLock_t* (*instantiate)(struct _ocrLockFactory_t* factory, ocrParamList_t *perInstance);

    ocrLockFcts_t lockFcts;
} ocrLockFactory_t;

/****************************************************/
/* OCR ATOMIC                                       */
/****************************************************/

struct _ocrAtomic64_t;

typedef struct {
    /**
     * @brief Destroy the atomic freeing up any
     * memory associated with it
     *
     * @param self          This atomic
     */
    void (*destruct)(struct _ocrAtomic64_t *self);

    /**
     * @brief Compare and swap
     *
     * The semantics are as follows (all operations performed atomically):
     *     - if location is cmpValue, atomically replace with
     *       newValue and return cmpValue
     *     - if location is *not* cmpValue, return value at location
     *
     * @param self          This atomic
     * @param cmpValue      Expected value of the atomic
     * @param newValue      Value to set if the atomic has the expected value
     *
     * @return Old value of the atomic
     */
    u64 (*cmpswap)(struct _ocrAtomic64_t *self, u64 cmpValue, u64 newValue);

    /**
     * @brief Atomic add
     *
     * The semantics are as follows (all operations performed atomically):
     *     - atomically increment location by addValue
     *     - return new value (after addition)
     *
     * @param self      This atomic
     * @param addValue  Value to add to location
     * @return New value of the location
     */
    u64 (*xadd)(struct _ocrAtomic64_t *self, u64 addValue);

    /**
     * @brief Return the current value
     *
     * This may return an "old" value if a concurrent
     * atomic change is happening.
     *
     * @param self      This atomic
     * @return Value of the atomic
     */
    u64 (*val)(struct _ocrAtomic64_t *self);
} ocrAtomic64Fcts_t;

/**
 * @brief A model for a simple atomic 64 bit value
 *
 * You can increment the atomic or perform a compare-and-swap
 *
 * @todo See if we need 32 bit, etc.
 */
typedef struct _ocrAtomic64_t {
    ocrAtomic64Fcts_t *fctPtrs;
} ocrAtomic64_t;

/****************************************************/
/* OCR ATOMIC 64 FACTORY                            */
/****************************************************/
/**
 * @brief Factory for atomics
 */
typedef struct _ocrAtomic64Factory_t {
    void (*destruct)(struct _ocrAtomic64Factory_t *self);

    ocrAtomic64_t* (*instantiate)(struct _ocrAtomic64Factory_t* factory, ocrParamList_t *perInstance);

    ocrAtomic64Fcts_t atomicFcts;
} ocrAtomic64Factory_t;

// TODO: Do we really need this?
/**
 * @brief Queue implementation.
 *
 * @todo Do we need to have multiple interfaces for different
 * types of queues
 */
struct _ocrQueue_t;

typedef struct _ocrQueueFcts_t {
    void (*destruct)(struct _ocrQueue_t *self);

    u64 (*popHead)(struct _ocrQueue_t *self);
    u64 (*popTail)(struct _ocrQueue_t *self);

    u64 (*pushHead)(struct _ocrQueue_t *self, u64 val);
    u64 (*pushTail)(struct _ocrQueue_t *self, u64 val);
} ocrQueueFcts_t;

typedef struct _ocrQueue_t {
    ocrQueueFcts_t *fctPtrs;
} ocrQueue_t;

/**
 * @brief Factory for queues
 */
typedef struct _ocrQueueFactory_t {
    void (*destruct)(struct _ocrQueueFactory_t *self);

    ocrQueue_t* (*instantiate)(struct _ocrQueueFactory_t *factory, ocrParamList_t* perType);

    ocrQueueFcts_t queueFcts;
} ocrQueueFactory_t;

#endif /* __OCR_SYNC_H__ */
