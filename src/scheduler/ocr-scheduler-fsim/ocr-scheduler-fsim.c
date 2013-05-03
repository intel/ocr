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

#include "ocr-runtime.h"
#include "fsim.h"

/******************************************************/
/* OCR-FSIM SCHEDULER                                 */
/******************************************************/

void xe_scheduler_destruct(ocr_scheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

ocr_workpile_t * xe_scheduler_pop_mapping_assert (ocr_scheduler_t* base, ocr_worker_t* w ) {
    assert( 0 && "Multiple XE pop mappings, do not use the class pop_mapping indirection");
    return NULL;
}

ocr_workpile_t * xe_scheduler_pop_mapping_to_assigned_work (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    size_t id = get_worker_id(w);
    assert(id >= hcDerived->worker_id_begin && id <= hcDerived->worker_id_end && "worker does not seem of this domain");
    return hcDerived->pools[ 2 * (id % hcDerived->n_workers_per_scheduler) ];
}

ocr_workpile_t * xe_scheduler_pop_mapping_to_work_shipping (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    // TODO sagnak; god awful hardcoding, BAD
    size_t id = hcDerived->worker_id_begin;
    return hcDerived->pools[ 1 + 2 * (id % hcDerived->n_workers_per_scheduler) ];
}

ocr_workpile_t * xe_scheduler_push_mapping_assert (ocr_scheduler_t* base, ocr_worker_t* w ) {
    assert( 0 && "Multiple XE push mappings, do not use the class push_mapping indirection");
    return NULL;
}

ocr_workpile_t * xe_scheduler_push_mapping_to_assigned_work (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    // TODO sagnak; god awful hardcoding, BAD
    size_t id = hcDerived->worker_id_begin;
    return hcDerived->pools[ 2 * (id % hcDerived->n_workers_per_scheduler) ];
}

ocr_workpile_t * xe_scheduler_push_mapping_to_work_shipping (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    size_t id = get_worker_id(w);
    assert(id >= hcDerived->worker_id_begin && id <= hcDerived->worker_id_end && "worker does not seem of this domain");
    return hcDerived->pools[ 1 + 2 * (id % hcDerived->n_workers_per_scheduler) ];
}

workpile_iterator_t* xe_scheduler_steal_mapping_fsim_faithful(ocr_scheduler_t* base, ocr_worker_t* w ) {
    assert( 0 && "XEs do not steal");
    return NULL;
}

// XE scheduler take can be called from two sites
// first by XE worker to execute work
ocrGuid_t xe_scheduler_take_most_fsim_faithful (ocr_scheduler_t* base, ocrGuid_t wid ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    ocr_workpile_t * wp_to_pop = NULL;

    size_t id = get_worker_id(w);
    if (id >= hcDerived->worker_id_begin && id <= hcDerived->worker_id_end) {
        // XE worker trying to extract 'executable work' from CE assigned workpile
        wp_to_pop = xe_scheduler_pop_mapping_to_assigned_work(base, w);
        // TODO sagnak this is true because XE and CE does not touch the workpile simultaneously
        // XE is woken up after the CE is done pushing the work here
        return wp_to_pop->pop(wp_to_pop);
    } else {
        // CE worker trying to extract buffered 'executable work' that XE encountered
        // TODO sagnak NOT IDEAL, and the XE may be simultaneously pushing, therefore we 'steal' for synchronization
        wp_to_pop = xe_scheduler_pop_mapping_to_work_shipping(base, w);
        return wp_to_pop->steal(wp_to_pop);
    }
    return NULL_GUID;
}

// XE scheduler give can be called from three sites:
// first being the ocrEdtSchedule, pushing user created work when executing user code
// those tasks should end up in the workpile designated for buffering work for the CE to take
// the second being, XE pushing a 'message task' which should be handed out to the CE policy domain
// third being the CE worker pushing work onto 'executable task' workpile
// the differentiation between {1,2} and {3} is through the worker id
// the differentiation between {1} and {2} is through a runtime check of the underlying task type
void xe_scheduler_give_fsim_faithful (ocr_scheduler_t* base, ocrGuid_t wid, ocrGuid_t tid ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    fsim_task_base_t* task = NULL;
    globalGuidProvider->getVal(globalGuidProvider, tid, (u64*)&task, NULL);

    fsim_message_interface_t* taskAsMessage = &(task->message_interface);

    size_t id = get_worker_id(w);
    if (id >= hcDerived->worker_id_begin && id <= hcDerived->worker_id_end) {
        // if the pusher is an XE worker
        if ( !taskAsMessage->is_message(taskAsMessage) ) {
            // TODO sagnak Multiple XE push mappings, do not use the class push_mapping indirection
            ocr_workpile_t * wp_to_push = xe_scheduler_push_mapping_to_work_shipping(base, w);
            wp_to_push->push(wp_to_push,tid);

            // TODO sagnak, this assumes (*A LOT*) the structure below, is this fair?
            // no assigned work found, now we have to create a 'message task'
            // by using our policy domain's message task factory
            ocr_policy_domain_t* policy_domain = base->domain;
            ocr_task_factory* message_task_factory = policy_domain->taskFactories[1];

            // the message to the CE says 'give me work' and notes who is asking for it
            ocrGuid_t messageTaskGuid = message_task_factory->create(message_task_factory, NULL, 0, NULL, NULL, 0);
            fsim_message_task_t* derivedMessage = NULL;
            globalGuidProvider->getVal(globalGuidProvider, messageTaskGuid, (u64*)&(derivedMessage), NULL);
            derivedMessage -> type = PICK_MY_WORK_UP;
            derivedMessage -> from_worker_guid = wid;

            // give the work to the XE scheduler, which in turn should give it to the CE
            // through policy domain hand out and the scheduler differentiates tasks by type (RTTI) like
            base->give(base, wid, messageTaskGuid);

        } else {
            ocr_policy_domain_t* xePolicyDomain = base->domain;
            xePolicyDomain->handOut(xePolicyDomain, wid, tid);
        }
    } else {
        // if the pusher is a CE worker
        ocr_workpile_t * wp_to_push = xe_scheduler_push_mapping_to_assigned_work(base, w);
        wp_to_push->push(wp_to_push,tid);
    }
}

ocr_scheduler_t* xe_scheduler_constructor() {
    xe_scheduler_t* derived = (xe_scheduler_t*) malloc(sizeof(xe_scheduler_t));
    ocr_scheduler_t* base = (ocr_scheduler_t*)derived;
    ocr_module_t * module_base = (ocr_module_t *) base;
    // module_base->map_fct = xe_ocr_module_map_workpiles_to_schedulers;
    module_base->map_fct = hc_ocr_module_map_workpiles_to_schedulers;
    base -> create = hc_scheduler_create;
    base -> destruct = xe_scheduler_destruct;
    base -> pop_mapping = xe_scheduler_pop_mapping_assert;
    base -> push_mapping = xe_scheduler_push_mapping_assert;
    base -> steal_mapping = xe_scheduler_steal_mapping_fsim_faithful;
    base -> take = xe_scheduler_take_most_fsim_faithful;
    base -> give = xe_scheduler_give_fsim_faithful;
    return base;
}

void ce_scheduler_destruct(ocr_scheduler_t * scheduler) {
    // just free self, workpiles are not allocated by the scheduler
    free(scheduler);
}

ocr_workpile_t * ce_scheduler_pop_mapping (ocr_scheduler_t* base, ocr_worker_t* w ) {
    ce_scheduler_t* ceDerived = (ce_scheduler_t*) base;
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    ocr_workpile_t * to_be_returned = NULL;

    size_t id = get_worker_id(w);
    if ( ceDerived -> in_message_popping_mode ) {
        // if I am the CE popping from my own message stash
        assert(id >= hcDerived->worker_id_begin && id <= hcDerived->worker_id_end && "worker does not seem of this domain");
        to_be_returned = hcDerived->pools[ 1 + 2 * (id % hcDerived->n_workers_per_scheduler) ];
    } else {
        // if I am the CE popping from my own work stash on behalf of XEs
        to_be_returned = hcDerived->pools[ 2 * (id % hcDerived->n_workers_per_scheduler) ];
    }
    return to_be_returned;
}

ocr_workpile_t * ce_scheduler_push_mapping_assert (ocr_scheduler_t* base, ocr_worker_t* w ) {
    assert( 0 && "Multiple CE push mappings, do not use the class push_mapping indirection");
    return NULL;
}

ocr_workpile_t * ce_scheduler_push_mapping_to_work (ocr_scheduler_t* base, ocr_worker_t* w ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    size_t id = get_worker_id(w);
    assert(id >= hcDerived->worker_id_begin && id <= hcDerived->worker_id_end && "worker does not seem of this domain");
    return hcDerived->pools[ 2 * (id % hcDerived->n_workers_per_scheduler) ];
}

ocr_workpile_t * ce_scheduler_push_mapping_to_messages (ocr_scheduler_t* base ) {
    hc_scheduler_t* hcDerived = (hc_scheduler_t*) base;
    // TODO sagnak; god awful hardcoding, BAD
    size_t id = hcDerived->worker_id_begin;
    return hcDerived->pools[ 1 + 2 * (id % hcDerived->n_workers_per_scheduler) ];
}

ocrGuid_t ce_scheduler_take (ocr_scheduler_t* base, ocrGuid_t wid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    ocr_workpile_t * wp_to_pop = base->pop_mapping(base, w);
    //ocrGuid_t popped = wp_to_pop->pop(wp_to_pop);
    // TODO fix synchronization errors
    ocrGuid_t popped = wp_to_pop->steal(wp_to_pop);

    return popped;
}

// this can be called from three different sites:
// one being the initial(from master) work that CE should dissipate
// other being the XEs giving a 'message task' for the CE to be notified
// last being the CEs giving a 'message task' to itself if it can not serve the message
void ce_scheduler_give (ocr_scheduler_t* base, ocrGuid_t wid, ocrGuid_t taskGuid ) {
    ocr_worker_t* w = NULL;
    globalGuidProvider->getVal(globalGuidProvider, wid, (u64*)&w, NULL);

    fsim_task_base_t* task = NULL;
    globalGuidProvider->getVal(globalGuidProvider, taskGuid, (u64*)&task, NULL);

    fsim_message_interface_t* taskAsMessage = &(task->message_interface);

    ocr_workpile_t * workpileToPush = NULL;

    if ( !taskAsMessage->is_message(taskAsMessage) ) {
        workpileToPush = ce_scheduler_push_mapping_to_work(base, w);
    } else {
        // TODO sagnak Multiple XE push mappings, do not use the class push_mapping indirection
        // TODO this is some foreign XE worker, what is the point in trickling down the worker id?
        // TODO sagnak this should be a LOCKED data structure
        // TODO there is no way to pick which 'message task pool' to go to for now :(
        workpileToPush = ce_scheduler_push_mapping_to_messages(base);
    }

    workpileToPush->push(workpileToPush,taskGuid);
}

workpile_iterator_t* ce_scheduler_steal_mapping_assert (ocr_scheduler_t* base, ocr_worker_t* w ) {
    assert( 0 && "CEs do not steal as of now");
    return NULL;
}

ocr_scheduler_t* ce_scheduler_constructor() {
    ce_scheduler_t* derived = (ce_scheduler_t*) malloc(sizeof(ce_scheduler_t));
    ocr_scheduler_t* base = (ocr_scheduler_t*)derived;
    ocr_module_t * module_base = (ocr_module_t *) base;
    module_base->map_fct = hc_ocr_module_map_workpiles_to_schedulers;
    base -> create = hc_scheduler_create;
    base -> destruct = ce_scheduler_destruct;
    base -> pop_mapping = ce_scheduler_pop_mapping;
    base -> push_mapping = ce_scheduler_push_mapping_assert;
    base -> steal_mapping = ce_scheduler_steal_mapping_assert;
    base -> take = ce_scheduler_take;
    base -> give = ce_scheduler_give;

    derived -> in_message_popping_mode = 1;
    return base;
}
