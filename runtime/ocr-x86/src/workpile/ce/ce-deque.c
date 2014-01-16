/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKPILE_CE

#include "ocr-hal.h"
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"
#include "workpile/hc/deque.h"


void dequeInit(ocrPolicyDomain_t *pd, deque_t * deq, void * init_value) {
    deq->head = 0;
    deq->tail = 0;
    deq->data = (volatile void **)pd->pdMalloc(pd, sizeof(void*)*INIT_DEQUE_CAPACITY);
    u32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = init_value;
        ++i;
    }
}

/*
 * push an entry onto the tail of the deque
 */
void dequePush(deque_t* deq, void* entry) {
    u32 head = deq->head;
    u32 tail = deq->tail;
    if (tail == INIT_DEQUE_CAPACITY + head) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        ASSERT("DEQUE full, increase deque's size" && 0);
    }
    u32 n = (deq->tail) % INIT_DEQUE_CAPACITY;
    deq->data[n] = entry;

    hal_fence();
    ++(deq->tail);
}

void dequeDestroy(ocrPolicyDomain_t *pd, deque_t* deq) {
    pd->pdFree(pd, deq->data);
}

/*
 * the steal protocol
 */
void * dequeSteal(deque_t * deq) {
    u32 head;
    /* Cannot read deq->data[head] here
     * Can happen that head=tail=0, then the owner of the deq pushes
     * a new task when stealer is here in the code, resulting in head=0, tail=1
     * All other checks down-below will be valid, but the old value of the buffer head
     * would be returned by the steal rather than the new pushed value.
     */
    u32 tail;
    
    head = deq->head;
    tail = deq->tail;
    if (tail <= head) {
        return NULL;
    }

    void * rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];

    /* compete with other thieves and possibly the owner (if the size == 1) */
    
    if (hal_cmpswap32(&deq->head, head, head + 1) == head) { /* competing */
        return rt;
    }
    return NULL;
}

/*
 * pop the task out of the deque from the tail
 */
void * dequePop(deque_t * deq) {
    hal_fence();
    u32 tail = deq->tail;
    ASSERT(tail > 0);
    --tail;
    deq->tail = tail;
    hal_fence();
    u32 head = deq->head;
    
    if (tail < head) {
        deq->tail = deq->head;
        return NULL;
    }
    void * rt = (void*) deq->data[(tail) % INIT_DEQUE_CAPACITY];

    if (tail > head) {
        return rt;
    }

    /* now size == 1, I need to compete with the thieves */
    if (hal_cmpswap32(&deq->head, head, head + 1) != head)
        rt = NULL; /* losing in competition */

    /* now the deque is empty */
    deq->tail = deq->head;
    return rt;
}


/******************************************************/
/* Semi Concurrent DEQUE                              */
/******************************************************/

void semiConcDequeInit(ocrPolicyDomain_t *pd, semiConcDeque_t* semiDeq, void * initValue) {
    deque_t* deq = (deque_t *) semiDeq;
    deq->head = 0;
    deq->tail = 0;
    semiDeq->lock = 0;
    deq->data = (volatile void **)pd->pdMalloc(pd, sizeof(void*)*INIT_DEQUE_CAPACITY);
    u32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = initValue;
        ++i;
    }
}

void semiConcDequeLockedPush(semiConcDeque_t* semiDeq, void* entry) {
    deque_t* deq = (deque_t *) semiDeq;
    u8 success = 0;
    while (!success) {
        u32 head = deq->head;
        u32 tail = deq->tail;
        if (INIT_DEQUE_CAPACITY + head == tail) {
            ASSERT("DEQUE full, increase deque's size" && 0);
        }

        if (hal_trylock32(&semiDeq->lock) == 0) {
            success = 1;
            deq->data[ deq->tail % INIT_DEQUE_CAPACITY ] = entry;
            hal_fence();
            ++deq->tail;
            hal_unlock32(&semiDeq->lock);
        }
    }
}

void * semiConcDequeNonLockedPop(semiConcDeque_t* semiDeq ) {
    deque_t* deq = (deque_t *) semiDeq;
    u32 head = deq->head;
    u32 tail = deq->tail;
    void * rt = NULL;

    if (tail > head) {
        rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];
        ++deq->head;
    }

    return rt;
}

void semiConcDequeDestroy(ocrPolicyDomain_t *pd, semiConcDeque_t *deq) {
    dequeDestroy(pd, (deque_t*)deq);
}

/******************************************************/
/* Non Concurrent DEQUE                               */
/******************************************************/

void nonConcDequeInit(ocrPolicyDomain_t *pd, nonConcDeque_t * deq, void * init_value) {
    deq->head = 0;
    deq->tail = 0;
    deq->data = (volatile void **)pd->pdMalloc(pd, sizeof(void*)*INIT_DEQUE_CAPACITY);
    u32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = init_value;
        ++i;
    }
}

void nonConcDequeDestroy(ocrPolicyDomain_t *pd, nonConcDeque_t* deq) {
    pd->pdFree(pd, deq->data);
}

/*
 * push an entry onto the tail of the deque
 */
void nonConcDequePush(nonConcDeque_t* deq, void* entry) {
    u32 head = deq->head;
    u32 tail = deq->tail;
    if (tail == INIT_DEQUE_CAPACITY + head) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        ASSERT("DEQUE full, increase deque's size" && 0);
    }
    u32 n = (deq->tail) % INIT_DEQUE_CAPACITY;
    deq->data[n] = entry;
    ++(deq->tail);
}

/*
 * pop the task out of the deque from the tail
 */
void * nonConcDequePopTail(nonConcDeque_t * deq) {
    ASSERT(deq->tail >= deq->head);
    if (deq->tail == deq->head)
        return NULL;
    --(deq->tail);
    void * rt = (void*) deq->data[(deq->tail) % INIT_DEQUE_CAPACITY];
    return rt;
}

/*
 * pop the task out of the deque from the head
 */
void * nonConcDequePopHead(nonConcDeque_t * deq) {
    ASSERT(deq->tail >= deq->head);
    if (deq->tail == deq->head)
        return NULL;
    void * rt = (void*) deq->data[(deq->head) % INIT_DEQUE_CAPACITY];
    ++(deq->head);
    return rt;
}

#endif /* ENABLE_WORKPILE_CE */
