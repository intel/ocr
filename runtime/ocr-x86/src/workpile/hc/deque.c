/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "debug.h"
#include "hc-sysdep.h"
#include "ocr-macros.h"
#include "ocr-types.h"
#include "workpile/hc/deque.h"

#include <stdlib.h>

void dequeInit(deque_t * deq, void * init_value) {
    deq->head = 0;
    deq->tail = 0;
    deq->data = (volatile void **) checkedMalloc(deq->data, sizeof(void*)*INIT_DEQUE_CAPACITY);
    s32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = init_value;
        i++;
    }
}

/*
 * push an entry onto the tail of the deque
 */
void dequePush(deque_t* deq, void* entry) {
    s32 size = deq->tail - deq->head;
    if (size == INIT_DEQUE_CAPACITY) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        ASSERT("DEQUE full, increase deque's size" && 0);
    }
    s32 n = (deq->tail) % INIT_DEQUE_CAPACITY;
    deq->data[n] = entry;

#ifdef __powerpc64__
    hc_mfence();
#endif
    deq->tail++;
}

void dequeDestroy(deque_t* deq) {
    free(deq->data);
    free(deq);
}

/*
 * the steal protocol
 */
void * dequeSteal(deque_t * deq) {
    s32 head;
    /* Cannot read deq->data[head] here
     * Can happen that head=tail=0, then the owner of the deq pushes
     * a new task when stealer is here in the code, resulting in head=0, tail=1
     * All other checks down-below will be valid, but the old value of the buffer head
     * would be returned by the steal rather than the new pushed value.
     */
    s32 tail;
    
    head = deq->head;
    tail = deq->tail;
    if ((tail - head) <= 0) {
        return NULL;
    }

    void * rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];

    /* compete with other thieves and possibly the owner (if the size == 1) */
    if (hc_cas(&deq->head, head, head + 1)) { /* competing */
        return rt;
    }
    return NULL;
}

/*
 * pop the task out of the deque from the tail
 */
void * dequePop(deque_t * deq) {
    hc_mfence();
    s32 tail = deq->tail;
    tail--;
    deq->tail = tail;
    hc_mfence();
    s32 head = deq->head;

    s32 size = tail - head;
    if (size < 0) {
        deq->tail = deq->head;
        return NULL;
    }
    void * rt = (void*) deq->data[(tail) % INIT_DEQUE_CAPACITY];

    if (size > 0) {
        return rt;
    }

    /* now size == 1, I need to compete with the thieves */
    if (!hc_cas(&deq->head, head, head + 1))
        rt = NULL; /* losing in competition */

    /* now the deque is empty */
    deq->tail = deq->head;
    return rt;
}


/******************************************************/
/* Semi Concurrent DEQUE                              */
/******************************************************/

void semiConcDequeInit(semiConcDeque_t* semiDeq, void * initValue) {
    deque_t* deq = (deque_t *) semiDeq;
    deq->head = 0;
    deq->tail = 0;
    semiDeq->lock = 0;
    deq->data = (volatile void **) malloc(sizeof(void*)*INIT_DEQUE_CAPACITY);
    s32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = initValue;
        i++;
    }
}

void semiConcDequeLockedPush(semiConcDeque_t* semiDeq, void* entry) {
    deque_t* deq = (deque_t *) semiDeq;
    s32 success = 0;
    while (!success) {
        s32 size = deq->tail - deq->head;
        if (INIT_DEQUE_CAPACITY == size) {
            ASSERT("DEQUE full, increase deque's size" && 0);
        }

        if (hc_cas(&semiDeq->lock, 0, 1) ) {
            success = 1;
            deq->data[ deq->tail % INIT_DEQUE_CAPACITY ] = entry;
            hc_mfence();
            ++deq->tail;
            semiDeq->lock= 0;
        }
    }
}

void * semiConcDequeNonLockedPop(semiConcDeque_t* semiDeq ) {
    deque_t* deq = (deque_t *) semiDeq;
    s32 head = deq->head;
    s32 tail = deq->tail;
    void * rt = NULL;

    if ((tail - head) > 0) {
        rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];
        ++deq->head;
    }

    return rt;
}

