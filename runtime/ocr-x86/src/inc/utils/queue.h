/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include "ocr-config.h"
#include "ocr-types.h"

/****************************************************/
/* QUEUE API                                         */
/****************************************************/

typedef struct _queue_t {
    struct _ocrPolicyDomain_t * pd;
    u32 size;
    u32 tail;
    void ** head;
} Queue_t;

void queueDestroy(Queue_t * queue);
u32 queueGetSize(Queue_t * queue);
bool queueIsEmpty(Queue_t * queue);
bool queueIsFull(Queue_t * queue);
void * queueGet(Queue_t * queue, u32 idx);
void queueAddLast(Queue_t * queue, void * elt);
void * queueRemoveLast(Queue_t * queue);
void * queueRemove(Queue_t * queue, u32 idx);
Queue_t * queueResize(Queue_t * queue, u32 newSize);
Queue_t * queueDoubleResize(Queue_t * queue, bool freeOld);

Queue_t * newBoundedQueue(struct _ocrPolicyDomain_t *pd, u32 size);

#endif /* QUEUE_H_ */
