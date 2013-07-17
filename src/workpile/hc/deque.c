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
    deq->buffer = (buffer_t *) checkedMalloc(deq->buffer, sizeof(buffer_t));
    deq->buffer->capacity = INIT_DEQUE_CAPACITY;
    deq->buffer->data = (volatile void **) checkedMalloc(deq->buffer->data, sizeof(void*)*INIT_DEQUE_CAPACITY);
    volatile void ** data = deq->buffer->data;
    s32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        data[i] = init_value;
        i++;
    }
}

/*
 * push an entry onto the tail of the deque
 */
void dequePush(deque_t* deq, void* entry) {
        s32 n = deq->buffer->capacity;
        s32 size = deq->tail - deq->head;
        if (size == n) { /* deque looks full */
                /* may not grow the deque if some interleaving steal occur */
                ASSERT("DEQUE full, increase deque's size" && 0);
        }
        n = (deq->tail) % deq->buffer->capacity;
        deq->buffer->data[n] = entry;

#ifdef __powerpc64__
        hc_mfence();
#endif
        deq->tail++;
}

void dequeDestroy(deque_t* deq) {
        free(deq->buffer->data);
        free(deq->buffer);
        free(deq);
}

void mpscDequeInit(mpsc_deque_t* deq, void * init_value) {
    deq->head = 0;
    deq->tail = 0;
    deq->push = 0;
    deq->buffer = (buffer_t *) malloc(sizeof(buffer_t));
    deq->buffer->capacity = INIT_DEQUE_CAPACITY;
    deq->buffer->data = (volatile void **) malloc(sizeof(void*)*INIT_DEQUE_CAPACITY);
    volatile void ** data = deq->buffer->data;
    s32 i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        data[i] = init_value;
        i++;
    }
}

void deque_locked_push(mpsc_deque_t* deq, void* entry) {
    s32 success = 0;
    s32 capacity = deq->buffer->capacity;

    while (!success) {
        s32 size = deq->tail - deq->head;
        if (capacity == size) {
            ASSERT("DEQUE full, increase deque's size" && 0);
        }

        if ( hc_cas(&deq->push, 0, 1) ) {
            success = 1;
            deq->buffer->data[ deq->tail % capacity ] = entry;
            hc_mfence();
            ++deq->tail;
            deq->push= 0;
        }
    }
}

void * deque_non_competing_pop_head (mpsc_deque_t* deq ) {
    s32 head = deq->head;
    s32 tail = deq->tail;
    void * rt = NULL;

    if ((tail - head) > 0) {
        rt = (void *) ((void **) deq->buffer->data)[head % deq->buffer->capacity];
        ++deq->head;
    }

    return rt;
}

/*
 * the steal protocol
 */
void * deque_steal(deque_t * deq) {
        s32 head;
        /* Cannot read deq->buffer[head] here
         * Can happen that head=tail=0, then the owner of the deq pushes
         * a new task when stealer is here in the code, resulting in head=0, tail=1
         * All other checks down-below will be valid, but the old value of the buffer head
         * would be returned by the steal rather than the new pushed value.
         */

        buffer_t * buffer;
        s32 tail;
        void * rt;

        ici:
        head = deq->head;
        tail = deq->tail;
        if ((tail - head) <= 0) {
                return NULL;
        }

        buffer = deq->buffer;
        rt = (void *) ((void **) buffer->data)[head % buffer->capacity];
        if(buffer != deq->buffer) {
                /* if buffer addr has changed the deque has been resized, we need to start over */
                goto ici;
        }

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
        s32 n = deq->buffer->capacity;

        s32 size = tail - head;
        if (size < 0) {
                deq->tail = deq->head;
                return NULL;
        }
        void * rt = (void*) deq->buffer->data[(tail) % n];

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
