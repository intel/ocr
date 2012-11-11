/* Copyright (c) 2012, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <assert.h>

#include "deque.h"
#include "hc_sysdep.h"

void deque_init(deque_t * deq, void * init_value) {
    deq->head = 0;
    deq->tail = 0;
    deq->buffer = (buffer_t *) malloc(sizeof(buffer_t));
    deq->buffer->capacity = INIT_DEQUE_CAPACITY;
    deq->buffer->data = (volatile void **) malloc(sizeof(void*)*INIT_DEQUE_CAPACITY);
    volatile void ** data = deq->buffer->data;
    int i=0;
    while(i < INIT_DEQUE_CAPACITY) {
        data[i] = init_value;
        i++;
    }
}

/*
 * push an entry onto the tail of the deque
 */
void deque_push(deque_t* deq, void* entry) {
	int n = deq->buffer->capacity;
	int size = deq->tail - deq->head;
	if (size == n) { /* deque looks full */
		/* may not grow the deque if some interleaving steal occur */
		assert("DEQUE full, increase deque's size" && 0);
	}
	n = (deq->tail) % deq->buffer->capacity;
	deq->buffer->data[n] = entry;

#ifdef __powerpc64__
	hc_mfence();
#endif 
	deq->tail++;
}

/*
 * the steal protocol
 */
void * deque_steal(deque_t * deq) {
	int head;
	/* Cannot read deq->buffer[head] here
	 * Can happen that head=tail=0, then the owner of the deq pushes
	 * a new task when stealer is here in the code, resulting in head=0, tail=1
	 * All other checks down-below will be valid, but the old value of the buffer head
	 * would be returned by the steal rather than the new pushed value.
	 */

	buffer_t * buffer;
	int tail;
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
void * deque_pop(deque_t * deq) {
	hc_mfence();
	int tail = deq->tail;
	tail--;
	deq->tail = tail;
	hc_mfence();
	int head = deq->head;
	int n = deq->buffer->capacity;

	int size = tail - head;
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
