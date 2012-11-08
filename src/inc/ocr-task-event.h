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

#ifndef __OCR_TASK_EVENT_H_
#define __OCR_TASK_EVENT_H_

#include "ocr-guid.h"
#include "ocr-edt.h"

struct ocr_event_struct;
typedef void (*event_destruct_fct)(struct ocr_event_struct* event);
typedef ocrGuid_t (*event_get_fct)(struct ocr_event_struct* event);
typedef void (*event_put_fct)(struct ocr_event_struct* event, ocrGuid_t db);
typedef bool (*event_register_if_not_ready_fct)(struct ocr_event_struct* event, ocrGuid_t polling_task_id );

/*! \brief Abstract class to represent OCR events.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  Events can be satisfied once with a GUID, can be polled for what GUID satisfied the event,
 *  and can be registered to by a task.
 */
typedef struct ocr_event_struct {
    /*! \brief Virtual destructor for the Event interface
     *  As this class does not have any state, the virtual destructor does not do anything
     */
    event_destruct_fct destruct;

    /*! \brief Interface to get the GUID of the entity that satisfied an event.
     *  \return GUID of the entity that satisfied this event
     */
    event_get_fct get;

    /*! \brief Interface to satisfy the event
     *  \param[in]  db  GUID to satisfy this event
     *  \param[in]  w   GUID of the Worker instance satisfying this event
     */
    event_put_fct put;

    /*! \brief Interface to register a task to this event
     *  \param[in]  polling_task_id GUID to Task that is registering to this event's satisfaction
     */
    event_register_if_not_ready_fct registerIfNotReady;
} ocr_event_t;

/*! \brief A linked list node data structure to wrap an OCR event and a successor.
*/
/*! \brief EventList linked list's node constructor
 *  \param[in]  pEvent  Event pointer to wrap to a linked list node
 *  \return A Node instance wrapping an Event to a linked list node
 */
typedef struct event_list_node_t {
    // TODO Node (ocr_event_t* pEvent) : event(pEvent), next(NULL) {}
    /*! Public Event member of the linked list node wrapper */
    ocr_event_t *event;
    /*! Public next Event member of the linked list node wrapper */
    struct event_list_node_t *next;
} event_list_node_t;

event_list_node_t* event_list_node_constructor ();
void event_list_node_destructor (event_list_node_t* node);

struct event_list_struct_t;
typedef void (*event_list_enlist_fct)( struct event_list_struct_t* list, ocrGuid_t event_guid );
/*! \brief A linked list data structure to list OCR events.
 *
 *  The runtime implementer or the end user may utilize this class to build a dynamic
 *  linked list of concrete Event implementation GUIDs to create dependency lists for a Task.
 */
typedef struct event_list_struct_t {
    /*! \brief Append an Event guid to the linked list
     *  \param[in]  event_guid  GUID of the event to be appended
     *
     *  This function extracts the Event object from the given guid, creates a linked list node
     *  wrapper by a Node instance, adds the node object to the tail of the linked list, and
     *  increments the size
     */
    event_list_enlist_fct enlist;
    /*! Public member to denote the current size of the linked list object */
    size_t size;
    /*! Public head and tail members of the linked list */
    event_list_node_t *head, *tail;
} event_list_t;

void event_list_enlist ( event_list_t* list, ocrGuid_t event_guid );

/*! \brief A linked list data structure constructor
 * It sets the size to be 0 and head and tail members to be NULL
 * \return  An empty linked list instance
 */
event_list_t* event_list_constructor ();

/*! \brief Virtual destructor for the EventList linked list data structure
 *  As this class does not have any state, the virtual destructor does not do anything
 */
void event_list_destructor ( event_list_t* list );

struct ocr_event_factory_struct;
typedef ocrGuid_t (*event_fact_create_fct) (struct ocr_event_factory_struct* factory, ocrEventTypes_t eventType, bool takesArg);
typedef void (*event_fact_destruct_fct)(struct ocr_event_factory_struct* factory);

/*! \brief Abstract factory class to create OCR events.
 *
 *  This class provides an interface to create Event instances with a non-static create function
 *  to allow runtime implementers to choose to have state in their derived ocr_event_factory classes.
 */
typedef struct ocr_event_factory_struct {
    /*! \brief Creates an concrete implementation of an Event and returns its corresponding GUID
     *  \return GUID of the concrete Event that is created by this call
     */
    event_fact_create_fct create;

    /*! \brief Virtual destructor for the ocr_event_factory interface
     *  As this class does not have any state, the virtual destructor does not do anything
     */
    event_fact_destruct_fct destruct;
} ocr_event_factory;

struct ocr_task_factory_struct;
typedef ocrGuid_t (*task_fact_create_with_event_list_fct) ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, event_list_t* al);
typedef ocrGuid_t (*task_fact_create_fct) ( struct ocr_task_factory_struct* factory, ocrEdt_t fctPtr, size_t l_size);
typedef void (*task_fact_destruct_fct)(struct ocr_task_factory_struct* factory);

/*! \brief Abstract factory class to create OCR tasks.
 *
 *  This class provides an interface to create Task instances with a non-static create function
 *  to allow runtime implementers to choose to have state in their derived TaskFactory classes.
 */
typedef struct ocr_task_factory_struct {
    /*! \brief Creates an concrete implementation of a Task and returns its corresponding GUID
     *  \param[in]  routine A user defined function that represents the computation this Task encapsulates.
     *  \param[in]  event_register_list Pointer to a linked list of events this Task instance depends on
     *  \param[in]  worker_id   The Worker instance creating this Task instance
     *  \return GUID of the concrete Task that is created by this call
     *
     *  The signature of the interface restricts the user computation that can be assigned to a task as follows.
     *  The user defined computation should take a vector of GUIDs and its size as their inputs, which may be
     *  the GUIDs used to satisfy the Events enlisted in the dependency list.
     *
     */
    task_fact_create_fct create;

    /*! \brief Virtual destructor for the TaskFactory interface
     *  As this class does not have any state, the virtual destructor does not do anything
     */
    task_fact_destruct_fct destruct;
} ocr_task_factory;

struct ocr_task_struct_t;
typedef void (*task_destruct_fct) ( struct ocr_task_struct_t* base );
typedef bool (*task_iterate_waiting_frontier_fct) ( struct ocr_task_struct_t* base );
typedef void (*task_execute_fct) ( struct ocr_task_struct_t* base );
typedef void (*task_schedule_fct) ( struct ocr_task_struct_t* base, ocrGuid_t wid );
typedef void (*task_add_dependency_fct) ( struct ocr_task_struct_t* base, ocr_event_t* dep, size_t index );
typedef u64  (*task_add_acquired_fct)( struct ocr_task_struct_t* base, u64 edtId, ocrGuid_t db);
typedef void (*task_remove_acquired_fct(struct ocr_task_struct_t* base, ocrGuid_t db, u64 dbId));

/*! \brief Abstract class to represent OCR tasks.
 *
 *  This class provides the interface for the underlying implementation to conform.
 *  OCR tasks can be executed and can have their synchronization frontier furthered by Events.
 */
typedef struct ocr_task_struct_t {
    /*! \brief Virtual destructor for the Task interface
     *  As this class does not have any state, the virtual destructor does not do anything
     */
    task_destruct_fct destruct;
    /*! \brief Interface to further synchronization frontier after a registered event satisfaction.
     *  \return true if Task instance is enabled, false otherwise
     */
    task_iterate_waiting_frontier_fct iterate_waiting_frontier;
    /*! \brief Interface to execute the underlying computation of a task
    */
    task_execute_fct execute;
    task_schedule_fct schedule;
    task_add_dependency_fct add_dependency;
    task_add_acquired_fct add_acquired;
    task_remove_acquired_fct remove_acquired;
} ocr_task_t;

////TODO old style factories
extern ocr_task_factory* taskFactory;
extern ocr_event_factory* eventFactory;

#endif /* __OCR_TASK_EVENT_H_ */
