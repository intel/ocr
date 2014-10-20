/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"

#include "debug.h"
#include "utils/queue.h"
#include "ocr-policy-domain.h"

#define DEBUG_TYPE UTIL

//
// Bounded Queue Implementation
//

Queue_t * newBoundedQueue(ocrPolicyDomain_t * pd, u32 size) {
    Queue_t * queue = (Queue_t *) pd->fcts.pdMalloc(pd, sizeof(Queue_t)+(size * sizeof(void*)));
    queue->pd = pd;
    queue->size = size;
    queue->tail = 0;
    queue->head = (void **) (((char*)queue)+sizeof(Queue_t));
    return queue;
}

void queueDestroy(Queue_t * queue) {
    ocrPolicyDomain_t * pd = queue->pd;
    pd->fcts.pdFree(pd, queue);
}

u32 queueGetSize(Queue_t * queue) {
    return queue->tail;
}

bool queueIsEmpty(Queue_t * queue) {
    return (queue == NULL) || (queue->tail == 0);
}

bool queueIsFull(Queue_t * queue) {
    return (queue->size == queue->tail);
}

void * queueGet(Queue_t * queue, u32 idx) {
    ASSERT(idx < queue->tail);
    return queue->head[idx];
}

void queueAddLast(Queue_t * queue, void * elt) {
    ASSERT(!queueIsFull(queue));
    queue->head[queue->tail++] = elt;
}

void * queueRemoveLast(Queue_t * queue) {
    ASSERT(!queueIsEmpty(queue));
    queue->tail--;
    return queue->head[queue->tail];
}

void * queueRemove(Queue_t * queue, u32 idx) {
    // TODO shrink queue
    ASSERT(idx < queue->tail);
    // Exchange last element and current
    void * elt = queue->head[idx];
    queue->tail--;
    // idx can equal tail, not an issue though
    queue->head[idx] = queue->head[queue->tail];
    return elt;
}

Queue_t * queueResize(Queue_t * queue, u32 newSize) {
    Queue_t * newQueue = newBoundedQueue(queue->pd, newSize);
    void * src = (void *) (((char*)queue)+sizeof(Queue_t));
    void * dst = (void *) (((char*)newQueue)+sizeof(Queue_t));
    hal_memCopy(dst, src, (queue->tail * sizeof(void*)), false);
    newQueue->tail = queue->tail;
    return newQueue;
}

Queue_t * queueDoubleResize(Queue_t * queue, bool freeOld) {
    Queue_t * newQueue = queueResize(queue, queue->size * 2);
    if (freeOld) {
        queueDestroy(queue);
    }
    return newQueue;
}


// void queueAddLastAutoResize(ocrPolicyDomain_t * pd, Queue_t ** queue, void * elt) {
//     if (tail == size) {
//         Queue_t * orgQueue = *queue;
//         Queue_t * newQueue = queueResize(pd, orgQueue, orgQueue->size * 2);
//         *queue = newQueue;
//     }
//     (*queue)->head[(*queue)->tail++] = elt;
// }