//
// Iterator Declaration
//
/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifndef LIST_H_
#define LIST_H_

#include "ocr-config.h"
#include "ocr-types.h"

/****************************************************/
/* ITERATOR API                                     */
/****************************************************/

typedef struct _iterator_t {
    struct _ocrPolicyDomain_t * pd;
    bool (*reset)(struct _iterator_t * it);
    bool (*hasNext)(struct _iterator_t * it);
    void * (*next)(struct _iterator_t * it);
    void (*removeCurrent)(struct _iterator_t * it);
    void (*destruct)(struct _iterator_t * it);
} iterator_t;

/****************************************************/
/* LIST API                                         */
/****************************************************/

struct _listnode;

typedef struct _linkedlist_t {
    struct _ocrPolicyDomain_t * pd;
    struct _listnode * head;
    struct _iterator_t * (*iterator) (struct _linkedlist_t *self);
    void (*pushFront)(struct _linkedlist_t *self, void * elt);
    bool (*isEmpty)(struct _linkedlist_t *self);
    void (*destruct)(struct _linkedlist_t *self);
} linkedlist_t;

struct _linkedlist_t * newLinkedList(struct _ocrPolicyDomain_t *pd);

#endif /* DEQUE_H_ */
