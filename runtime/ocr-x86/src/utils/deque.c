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
#include "utils/deque.h"


#define DEBUG_TYPE UTIL

/****************************************************/
/* DEQUE INIT and DESTROY                           */
/****************************************************/
/*
 * Deque destroy
 */
void dequeDestroy(ocrPolicyDomain_t *pd, deque_t* deq) {
    pd->pdFree(pd, deq->data);
}

static void baseDequeInit(deque_t* deq, ocrPolicyDomain_t *pd, void * initValue) {
    deq->head = 0;
    deq->tail = 0;
    deq->data = (volatile void **)pd->pdMalloc(pd, sizeof(void*)*INIT_DEQUE_CAPACITY);
    u32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = initValue;
        ++i;
    }
    deq->destruct = dequeDestroy;
    deq->pushAtTail = NULL;
    deq->popFromTail = NULL;
    deq->pushAtHead = NULL;
    deq->popFromHead = NULL;
}

static void singleLockedDequeInit(dequeSingleLocked_t* deq, ocrPolicyDomain_t *pd, void * initValue) {
    baseDequeInit((deque_t*)deq, pd, initValue);
    deq->lock = 0;
}

static void dualLockedDequeInit(dequeDualLocked_t* deq, ocrPolicyDomain_t *pd, void * initValue) {
    baseDequeInit((deque_t*)deq, pd, initValue);
    deq->lockH = 0;
    deq->lockT = 0;
}

deque_t* dequeInit(ocrPolicyDomain_t *pd, void * initValue, ocrDequeType_t type) {
    deque_t* deq = NULL;
    switch(type) {
      case BASE_DEQUETYPE:
        deq = (deque_t*)pd->pdMalloc(pd, sizeof(deque_t));
        baseDequeInit(deq, pd, initValue);
        break;
      case SINGLE_LOCKED_DEQUETYPE:
        deq = (deque_t*)pd->pdMalloc(pd, sizeof(dequeSingleLocked_t));
        singleLockedDequeInit((dequeSingleLocked_t*)deq, pd, initValue);
        break;
      case DUAL_LOCKED_DEQUETYPE:
        deq = (deque_t*)pd->pdMalloc(pd, sizeof(dequeDualLocked_t));
        dualLockedDequeInit((dequeDualLocked_t*)deq, pd, initValue);
        break;
      default:
        ASSERT(0);
    }
    deq->type = type;
    return deq;
}

/****************************************************/
/* NON CONCURRENT DEQUE                             */
/****************************************************/

/*
 * push an entry onto the tail of the deque
 */
void nonConcDequePushTail(deque_t* deq, void* entry, u8 doTry) {
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
void * nonConcDequePopTail(deque_t * deq, u8 doTry) {
    ASSERT(deq->tail >= deq->head);
    if (deq->tail == deq->head)
        return NULL;
    --(deq->tail);
    void * rt = (void*) deq->data[(deq->tail) % INIT_DEQUE_CAPACITY];
    return rt;
}

/*
 *  pop the task out of the deque from the head
 */
void * nonConcDequePopHead(deque_t * deq, u8 doTry) {
    ASSERT(deq->tail >= deq->head);
    if (deq->tail == deq->head)
        return NULL;
    void * rt = (void*) deq->data[(deq->head) % INIT_DEQUE_CAPACITY];
    ++(deq->head);
    return rt;
}

/****************************************************/
/* CONCURRENT DEQUE                                 */
/****************************************************/
/*
 * push an entry onto the tail of the deque
 */
void concDequePushTail(deque_t* deq, void* entry, u8 doTry) {
    s32 head = deq->head;
    s32 tail = deq->tail;
    DPRINTF(DEBUG_LVL_VERB, "Pushing 0x%lx into conc deque @ 0x%lx\n",
            (u64)entry, (u64)deq);
    if (tail == INIT_DEQUE_CAPACITY + head) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        ASSERT("DEQUE full, increase deque's size" && 0);
    }
    s32 n = (deq->tail) % INIT_DEQUE_CAPACITY;
    deq->data[n] = entry;

    hal_fence();
    ++(deq->tail);
}

/*
 * pop the task out of the deque from the tail
 */
void * concDequePopTail(deque_t * deq, u8 doTry) {
    hal_fence();
    s32 tail = deq->tail;
    --tail;
    deq->tail = tail;
    hal_fence();
    s32 head = deq->head;
    
    if (tail < head) {
        deq->tail = deq->head;
        return NULL;
    }
    void * rt = (void*) deq->data[(tail) % INIT_DEQUE_CAPACITY];

    if (tail > head) {
        DPRINTF(DEBUG_LVL_VERB, "Popping (tail) 0x%lx from conc deque @ 0x%lx\n",
                (u64)rt, (u64)deq);
        return rt;
    }

    /* now size == 1, I need to compete with the thieves */
    if (hal_cmpswap32(&deq->head, head, head + 1) != head)
        rt = NULL; /* losing in competition */

    /* now the deque is empty */
    deq->tail = deq->head;
    DPRINTF(DEBUG_LVL_VERB, "Popping (tail 2) 0x%lx from conc deque @ 0x%lx\n",
            (u64)rt, (u64)deq);
    return rt;
}

/*
 * the steal protocol
 */
void * concDequePopHead(deque_t * deq, u8 doTry) {
    s32 head, tail;
    do {
        tail = deq->tail;
        hal_fence();
        head = deq->head;
        if (tail <= head) {
            return NULL;
        }

        void * rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];

        /* compete with other thieves and possibly the owner (if the size == 1) */
    
        if (hal_cmpswap32(&deq->head, head, head + 1) == head) { /* competing */
            DPRINTF(DEBUG_LVL_VERB, "Popping (head) 0x%lx from conc deque @ 0x%lx\n",
                    (u64)rt, (u64)deq);
            return rt;
        }
    } while (doTry == 0);
    return NULL;
}

/******************************************************/
/* SINGLE LOCKED DEQUE                                */
/******************************************************/

void singleLockedDequePushTail(deque_t* deq, void* entry, u8 doTry) {
    dequeSingleLocked_t* semiDeq = (dequeSingleLocked_t*)deq;
    do {
        s32 head = deq->head;
        s32 tail = deq->tail;
        if (INIT_DEQUE_CAPACITY + head == tail) {
            ASSERT("DEQUE full, increase deque's size" && 0);
        }

        if (hal_trylock32(&semiDeq->lock) == 0) {
            deq->data[ deq->tail % INIT_DEQUE_CAPACITY ] = entry;
            hal_fence();
            ++deq->tail;
            hal_unlock32(&semiDeq->lock);
            return;
        }
    } while(doTry == 0);
}

/******************************************************/
/* DUAL LOCKED DEQUE                                  */
/******************************************************/

/******************************************************/
/* DEQUE API                                          */
/******************************************************/
deque_t* newWorkStealingDeque(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = dequeInit(pd, initValue, BASE_DEQUETYPE);
    deq->pushAtTail = concDequePushTail;
    deq->popFromTail = concDequePopTail;
    deq->pushAtHead = NULL;
    deq->popFromHead = concDequePopHead;
    return deq;
}

deque_t* newNonConcurrentQueue(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = dequeInit(pd, initValue, BASE_DEQUETYPE);
    deq->pushAtTail = nonConcDequePushTail;
    deq->popFromTail = nonConcDequePopTail;
    deq->pushAtHead = NULL;
    deq->popFromHead = nonConcDequePopHead;
    return deq;
}

deque_t* newSemiConcurrentQueue(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = dequeInit(pd, initValue, SINGLE_LOCKED_DEQUETYPE);
    deq->pushAtTail = singleLockedDequePushTail;
    deq->popFromTail = NULL;
    deq->pushAtHead = NULL;
    deq->popFromHead = nonConcDequePopHead;
    return deq;
}

