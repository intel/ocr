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
#include "worker/hc/hc-worker.h"

#define DEBUG_TYPE UTIL

/****************************************************/
/* DEQUE BASE IMPLEMENTATIONS                       */
/****************************************************/

/*
 * Deque destroy
 */
void dequeDestroy(ocrPolicyDomain_t *pd, deque_t* deq) {
    pd->fcts.pdFree(pd, deq->data);
    pd->fcts.pdFree(pd, deq);
}

/*
 * @brief base constructor for deques. Think of this as a virtual class
 * where the function pointers to push and pop are set by the derived
 * implementation.
 */
static void baseDequeInit(deque_t* deq, ocrPolicyDomain_t *pd, void * initValue) {
    deq->head = 0;
    deq->tail = 0;
    deq->data = (volatile void **)pd->fcts.pdMalloc(pd, sizeof(void*)*INIT_DEQUE_CAPACITY);
    u32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        deq->data[i] = initValue;
        ++i;
    }
    deq->destruct = dequeDestroy;
    // Set by derived implementation
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

static deque_t * newBaseDeque(ocrPolicyDomain_t *pd, void * initValue, ocrDequeType_t type) {
    deque_t* deq = NULL;
    switch(type) {
        case NO_LOCK_BASE_DEQUE:
            deq = (deque_t*) pd->fcts.pdMalloc(pd, sizeof(deque_t));
            baseDequeInit(deq, pd, initValue);
            // Warning: function pointers must be specialized in caller
            break;
        case SINGLE_LOCK_BASE_DEQUE:
            deq = (deque_t*) pd->fcts.pdMalloc(pd, sizeof(dequeSingleLocked_t));
            singleLockedDequeInit((dequeSingleLocked_t*)deq, pd, initValue);
            // Warning: function pointers must be specialized in caller
            break;
        case DUAL_LOCK_BASE_DEQUE:
            deq = (deque_t*) pd->fcts.pdMalloc(pd, sizeof(dequeDualLocked_t));
            dualLockedDequeInit((dequeDualLocked_t*)deq, pd, initValue);
            // Warning: function pointers must be specialized in caller
            break;
    default:
        ASSERT(0);
    }
    deq->type = type;
    return deq;
}

/****************************************************/
/* NON CONCURRENT DEQUE BASED OPERATIONS            */
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
/* CONCURRENT DEQUE BASED OPERATIONS                */
/****************************************************/

/*
 * push an entry onto the tail of the deque
 */
void wstDequePushTail(deque_t* deq, void* entry, u8 doTry) {
    s32 head = deq->head;
    s32 tail = deq->tail;
    if (tail == INIT_DEQUE_CAPACITY + head) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        ASSERT("DEQUE full, increase deque's size" && 0);
    }
    s32 n = (deq->tail) % INIT_DEQUE_CAPACITY;
    deq->data[n] = entry;
    DPRINTF(DEBUG_LVL_VERB, "Pushing h:%d t:%d deq[%d] elt:0x%lx into conc deque @ 0x%lx\n",
            head, deq->tail, n, (u64)entry, (u64)deq);
    hal_fence();
    ++(deq->tail);
}

/*
 * pop the task out of the deque from the tail
 */
void * wstDequePopTail(deque_t * deq, u8 doTry) {
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
        DPRINTF(DEBUG_LVL_VERB, "Popping (tail) h:%d t:%d deq[%d] elt:0x%lx from conc deque @ 0x%lx\n",
                head, tail, tail % INIT_DEQUE_CAPACITY, (u64)rt, (u64)deq);
        return rt;
    }

    /* now size == 1, I need to compete with the thieves */
    if (hal_cmpswap32(&deq->head, head, head + 1) != head)
        rt = NULL; /* losing in competition */

    /* now the deque is empty */
    deq->tail = deq->head;
    DPRINTF(DEBUG_LVL_VERB, "Popping (tail 2) h:%d t:%d deq[%d] elt:0x%lx from conc deque @ 0x%lx\n",
            head, tail, tail % INIT_DEQUE_CAPACITY, (u64)rt, (u64)deq);
    return rt;
}

/*
 * the steal protocol
 */
void * wstDequePopHead(deque_t * deq, u8 doTry) {
    s32 head, tail;
    do {
        head = deq->head;
        hal_fence();
        tail = deq->tail;
        if (tail <= head) {
            return NULL;
        }

        // The data must be read here, BEFORE the cas succeeds.
        // If the tail wraps around the buffer, so that H=x and T=H+N
        // as soon as the steal has done the cas, a push could happen
        // at index 'x' and overwrite the value to be stolen.

        void * rt = (void *) deq->data[head % INIT_DEQUE_CAPACITY];

        /* compete with other thieves and possibly the owner (if the size == 1) */
        if (hal_cmpswap32(&deq->head, head, head + 1) == head) { /* competing */
            DPRINTF(DEBUG_LVL_VERB, "Popping (head) h:%d t:%d deq[%d] elt:0x%lx from conc deque @ 0x%lx\n",
                     head, tail, head % INIT_DEQUE_CAPACITY, (u64)rt, (u64)deq);
            return rt;
        }
    } while (doTry == 0);
    return NULL;
}

/******************************************************/
/* SINGLE LOCKED DEQUE BASED OPERATIONS               */
/******************************************************/

/*
 * Push an entry onto the tail of the deque
 * This operation locks the whole deque.
 */
void lockedDequePushTail(deque_t* self, void* entry, u8 doTry) {
    dequeSingleLocked_t* dself = (dequeSingleLocked_t*)self;
    hal_lock32(&dself->lock);
    u32 head = self->head;
    u32 tail = self->tail;
    if (tail == INIT_DEQUE_CAPACITY + head) { /* deque looks full */
        /* may not grow the deque if some interleaving steal occur */
        ASSERT("DEQUE full, increase deque's size" && 0);
    }
    u32 n = (self->tail) % INIT_DEQUE_CAPACITY;
    self->data[n] = entry;
    ++(self->tail);
    hal_unlock32(&dself->lock);
}

/*
 * Pop the task out of the deque from the tail
 * This operation locks the whole deque.
 */
void * lockedDequePopTail(deque_t * self, u8 doTry) {
    dequeSingleLocked_t* dself = (dequeSingleLocked_t*)self;
    hal_lock32(&dself->lock);
    ASSERT(self->tail >= self->head);
    if (self->tail == self->head) {
        hal_unlock32(&dself->lock);
        return NULL;
    }
    --(self->tail);
    void * rt = (void*) self->data[(self->tail) % INIT_DEQUE_CAPACITY];
    hal_unlock32(&dself->lock);
    return rt;
}

/*
 * Pop the task out of the deque from the head
 * This operation locks the whole deque.
 */
void * lockedDequePopHead(deque_t * self, u8 doTry) {
    dequeSingleLocked_t* dself = (dequeSingleLocked_t*)self;
    hal_lock32(&dself->lock);
    ASSERT(self->tail >= self->head);
    if (self->tail == self->head) {
        hal_unlock32(&dself->lock);
        return NULL;
    }
    void * rt = (void*) self->data[(self->head) % INIT_DEQUE_CAPACITY];
    ++(self->head);
    hal_unlock32(&dself->lock);
    return rt;
}

/******************************************************/
/* DEQUE CONSTRUCTORS                                 */
/******************************************************/

/*
 * @brief Deque constructor. For a given type, create an instance and
 * initialize its base type.
 */
static deque_t * newDeque(ocrPolicyDomain_t *pd, void * initValue, ocrDequeType_t type) {
    deque_t* deq = NULL;
    switch(type) {
    case WORK_STEALING_DEQUE:
        deq = newBaseDeque(pd, initValue, NO_LOCK_BASE_DEQUE);
        // Specialize push/pop implementations
        deq->pushAtTail = wstDequePushTail;
        deq->popFromTail = wstDequePopTail;
        deq->pushAtHead = NULL;
        deq->popFromHead = wstDequePopHead;
        break;
    case NON_CONCURRENT_DEQUE:
        deq = newBaseDeque(pd, initValue, NO_LOCK_BASE_DEQUE);
        // Specialize push/pop implementations
        deq->pushAtTail = nonConcDequePushTail;
        deq->popFromTail = nonConcDequePopTail;
        deq->pushAtHead = NULL;
        deq->popFromHead = nonConcDequePopHead;
        break;
    case SEMI_CONCURRENT_DEQUE:
        deq = newBaseDeque(pd, initValue, SINGLE_LOCK_BASE_DEQUE);
        // Specialize push/pop implementations
        deq->pushAtTail = lockedDequePushTail;
        deq->popFromTail = NULL;
        deq->pushAtHead = NULL;
        deq->popFromHead = nonConcDequePopHead;
        break;
    case LOCKED_DEQUE:
        deq = newBaseDeque(pd, initValue, SINGLE_LOCK_BASE_DEQUE);
        // Specialize push/pop implementations
        deq->pushAtTail =  lockedDequePushTail;
        deq->popFromTail = lockedDequePopTail;
        deq->pushAtHead = NULL;
        deq->popFromHead = lockedDequePopHead;
        break;
    default:
        ASSERT(0);
    }
    deq->type = type;
    return deq;
}

/**
 * @brief The workstealing deque is a concurrent deque that supports
 * push at the tail, and pop from either tail or head. Popping from the
 * head is usually called a steal.
 */
deque_t* newWorkStealingDeque(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = newDeque(pd, initValue, WORK_STEALING_DEQUE);
    return deq;
}

/**
 * @brief Unsynchronized implementation for push and pop
 */
deque_t* newNonConcurrentQueue(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = newDeque(pd, initValue, NON_CONCURRENT_DEQUE);
    return deq;
}

/**
 * @brief Allows multiple concurrent push at the tail (a single succeed
 at a time, others fail and need to retry). Pop from head are not synchronized.
 */
deque_t* newSemiConcurrentQueue(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = newDeque(pd, initValue, SEMI_CONCURRENT_DEQUE);
    return deq;
}

/**
 * @brief Allows multiple concurrent push and pop. All operations are serialized.
 */
deque_t* newLockedQueue(ocrPolicyDomain_t *pd, void * initValue) {
    deque_t* deq = newDeque(pd, initValue, WORK_STEALING_DEQUE);
    return deq;
}


