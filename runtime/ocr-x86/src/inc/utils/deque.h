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
 *
 */
typedef enum {
    BASE_DEQUETYPE           = 0x1, // Basic deque
    SINGLE_LOCKED_DEQUETYPE  = 0x2, // deque with single lock
    DUAL_LOCKED_DEQUETYPE    = 0x3, // deque with dual lock
    MAX_DEQUETYPE            = 0x4
} ocrDequeType_t;

/****************************************************/
/* BASE DEQUE                                       */
/****************************************************/

/**
 * @brief Deques are double ended queues. They have a head and a tail.
 * In a typical queue, new elements are pushed at the 
 * tail, while being popped from the head of the queue.
 * Many variations exist, one being the workstealing deque. The 
 * workstealing deque is a concurrent deque that supports push 
 * at the tail, and pop from either tail or head. Popping from the
 * head is usually called a steal.
 *
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

typedef struct _ocrDequeSingleLocked_t {
    deque_t base;
    volatile u32 lock;
} dequeSingleLocked_t;

/****************************************************/
/* DUAL LOCKED DEQUE                                */
/****************************************************/

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

#endif /* DEQUE_H_ */
