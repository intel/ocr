/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef DEQUE_H_
#define DEQUE_H_


/****************************************************/
/* DEQUE API                                        */
/****************************************************/

typedef struct {
    volatile int head;
    volatile int tail;
    volatile void ** data;
} deque_t;

#ifndef INIT_DEQUE_CAPACITY
// Set by configure
#define INIT_DEQUE_CAPACITY 128
#endif

void dequeInit(deque_t * deq, void * initValue);
void * dequeSteal(deque_t * deq);
void dequePush(deque_t* deq, void* entry);
void * dequePop(deque_t * deq);
void dequeDestroy(deque_t* deq);


/****************************************************/
/* Semi Concurrent DEQUE API                        */
/****************************************************/

typedef struct {
    deque_t deque;    
    volatile int lock;
} semiConcDeque_t;

void semiConcDequeInit(semiConcDeque_t* deq, void * initValue);
void* semiConcDequeNonLockedPop (semiConcDeque_t* deq );
void semiConcDequeLockedPush(semiConcDeque_t* deq, void* entry);

#endif /* DEQUE_H_ */
