/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef DEQUE_H_
#define DEQUE_H_

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_CE

#include "ocr-types.h"

struct _ocrPolicyDomain_t;

/****************************************************/
/* DEQUE API                                        */
/****************************************************/

typedef struct {
    volatile u32 head;
    volatile u32 tail;
    volatile void ** data;
} deque_t;

#ifndef INIT_DEQUE_CAPACITY
// Set by configure
#define INIT_DEQUE_CAPACITY 128
#endif

void dequeInit(struct _ocrPolicyDomain_t *pd, deque_t * deq, void * initValue);
void * dequeSteal(deque_t * deq);
void dequePush(deque_t* deq, void* entry);
void * dequePop(deque_t * deq);
// Does not destroy the deque_t but just supporting
// structures
void dequeDestroy(struct _ocrPolicyDomain_t *pd, deque_t* deq);


/****************************************************/
/* Semi Concurrent DEQUE API                        */
/****************************************************/

typedef struct {
    deque_t deque;    
    volatile u32 lock;
} semiConcDeque_t;

void semiConcDequeInit(struct _ocrPolicyDomain_t *pd, semiConcDeque_t* deq,
                       void * initValue);
void* semiConcDequeNonLockedPop (semiConcDeque_t* deq );
void semiConcDequeLockedPush(semiConcDeque_t* deq, void* entry);
// Does not destroy the semi-concurrent Deque but just supporting
// structures
void semiConcDequeDestroy(struct _ocrPolicyDomain_t *pd, semiConcDeque_t *deq);

/****************************************************/
/* Non Concurrent DEQUE API                         */
/****************************************************/

typedef struct {
    u32 head;
    u32 tail;
    volatile void ** data;
} nonConcDeque_t;

void nonConcDequeInit(ocrPolicyDomain_t *pd, nonConcDeque_t * deq, void * init_value);
void nonConcDequeDestroy(ocrPolicyDomain_t *pd, nonConcDeque_t* deq);
void nonConcDequePush(nonConcDeque_t* deq, void* entry);
void* nonConcDequePopHead (nonConcDeque_t* deq );
void* nonConcDequePopTail (nonConcDeque_t* deq );

#endif /* ENABLE_WORKPILE_CE */
#endif /* DEQUE_H_ */
