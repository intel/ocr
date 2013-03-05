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

#include "ocr-macros.h"
#include "ocr-runtime.h"
#include "ocr-guid.h"

/*
 * @file This file contains OCR Event list construction public API implementation.
 */

/*! \brief A linked list data-structure constructor
 * It sets the size to be 0 and head and tail members to be NULL
 * \return  An empty linked list instance
 */
static event_list_node_t* event_list_node_constructor () {
    event_list_node_t* node = (event_list_node_t*)checked_malloc(node, sizeof(event_list_node_t));
    node->event = NULL;
    node->next = NULL;
    return node;
}

/*! \brief Shallow destructor for event list nodes
 *  \param[in] node The node to free.
 */
static void event_list_node_destructor (event_list_node_t* node) {
    free(node);
}

/*! \brief Append an Event guid to the event list
 *  \param[in]  list An event list
 *  \param[in]  event_guid  GUID of the event to be appended
 */
void event_list_enlist ( event_list_t* list, ocrGuid_t event_guid ) {
    ocr_event_t* event = (ocr_event_t*) deguidify(event_guid);
    ++list->size;
    event_list_node_t* node = event_list_node_constructor();
    node->event = event;
    if ( NULL != list->head ) { list->tail->next = node; list->tail = node; }
    else list->tail = list->head = node;
}

/*! \brief Default constructor for event list
 * Initializes an empty list and register the enlist protocol
 * through a function pointer to 'event_list_enlist'
 */
event_list_t* event_list_constructor () {
    event_list_t* list = (event_list_t*)checked_malloc(list, sizeof(event_list_t));
    list->size = 0;
    list->head = list->tail = NULL;
    list->enlist = event_list_enlist;
    return list;
}

/*! \brief free an event list
 * Goes over the list and free each of the node.
 * Warning: the 'list' parameter is freed too !
 */
void event_list_destructor ( event_list_t* list ) {
    event_list_node_t* curr = list->head->next;
    while ( NULL != list->head ) {
        event_list_node_destructor(list->head);
        list->head = curr;
        curr = curr->next;
    }
    free(list);
}
