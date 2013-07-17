/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef DEQUE_H_
#define DEQUE_H_

typedef struct buffer {
        int capacity;
        volatile void ** data;
} buffer_t;

typedef struct deque {
        volatile int head;
        volatile int tail;
        buffer_t * buffer;
} deque_t;

#define INIT_DEQUE_CAPACITY 128
#define SLOW_EXPAND_THRESHOLD 128
#define INC_DEQUE_CAPACITY 64

void dequeInit(deque_t * deq, void * init_value);
void * deque_steal(deque_t * deq);
void dequePush(deque_t* deq, void* entry);
void * dequePop(deque_t * deq);
void dequeDestroy(deque_t* deq);

typedef struct locked_deque {
        volatile int head;
        volatile int tail;
        volatile int push;
        buffer_t * buffer;
} mpsc_deque_t;

void mpscDequeInit(mpsc_deque_t* deq, void * init_value);
void* deque_non_competing_pop_head (mpsc_deque_t* deq );
void deque_locked_push(mpsc_deque_t* deq, void* entry);

#endif /* DEQUE_H_ */
