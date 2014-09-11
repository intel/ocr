/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef DEQUE_H_
#define DEQUE_H_

#include "ocr-config.h"
#include "ocr-types.h"
#include "ocr-policy-domain.h"

#ifndef INIT_DEQUE_CAPACITY
// Set by configure
#define INIT_DEQUE_CAPACITY 32768
#endif

/****************************************************/
/* DEQUE TYPES                                      */
/****************************************************/

/**
 * @brief Type of deques
 */
typedef enum {
    // Virtual implementations
    NO_LOCK_BASE_DEQUE       = 0x1,
    SINGLE_LOCK_BASE_DEQUE   = 0x2,
    DUAL_LOCK_BASE_DEQUE     = 0x3,
    // Concrete implementations
    WORK_STEALING_DEQUE      = 0x4,
    NON_CONCURRENT_DEQUE     = 0x5,
    SEMI_CONCURRENT_DEQUE    = 0x6,
    LOCKED_DEQUE             = 0x7,
    MAX_DEQUETYPE            = 0x8
} ocrDequeType_t;

/****************************************************/
/* BASE DEQUE                                       */
/****************************************************/

/**
 * @brief Deques are double ended queues. They have a head and a tail.
 * In a typical queue, new elements are pushed at the
 * tail, while being popped from the head of the queue.
 */
typedef struct _ocrDeque_t {
    ocrDequeType_t type;
    volatile s32 head;
    volatile s32 tail;
    volatile void ** data;

    /** @brief Destruct deque
     */
    void (*destruct)(ocrPolicyDomain_t *pd, struct _ocrDeque_t *self);

    /** @brief Push element at tail
     */
    void (*pushAtTail)(struct _ocrDeque_t *self, void* entry, u8 doTry);

    /** @brief Pop element from tail
     */
    void* (*popFromTail)(struct _ocrDeque_t *self, u8 doTry);

    /** @brief Push element at head
     */
    void (*pushAtHead)(struct _ocrDeque_t *self, void* entry, u8 doTry);

    /** @brief Pop element from head
     */
    void* (*popFromHead)(struct _ocrDeque_t *self, u8 doTry);
} deque_t;

/****************************************************/
/* SINGLE LOCKED DEQUE                              */
/****************************************************/

// deque with single lock
typedef struct _ocrDequeSingleLocked_t {
    deque_t base;
    volatile u32 lock;
} dequeSingleLocked_t;

/****************************************************/
/* DUAL LOCKED DEQUE                                */
/****************************************************/

// deque with dual lock
typedef struct _ocrDequeDualLocked_t {
    deque_t base;
    volatile u32 lockH;
    volatile u32 lockT;
} dequeDualLocked_t;

/****************************************************/
/* DEQUE API                                        */
/****************************************************/

deque_t* newWorkStealingDeque(ocrPolicyDomain_t *pd, void * initValue);
deque_t* newNonConcurrentQueue(ocrPolicyDomain_t *pd, void * initValue);
deque_t* newSemiConcurrentQueue(ocrPolicyDomain_t *pd, void * initValue);
deque_t* newLockedQueue(ocrPolicyDomain_t *pd, void * initValue);

#endif /* DEQUE_H_ */
