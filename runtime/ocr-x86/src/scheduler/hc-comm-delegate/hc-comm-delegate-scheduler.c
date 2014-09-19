/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SCHEDULER_HC_COMM_DELEGATE

#include "debug.h"
#include "ocr-sysboot.h"
#include "utils/deque.h"
#include "utils/list.h"
#include "worker/hc/hc-worker.h"
#include "scheduler/hc-comm-delegate/hc-comm-delegate-scheduler.h"
#include "comm-api/delegate/delegate-comm-api.h"

#define DEBUG_TYPE SCHEDULER

/**
 * Extensions to the HC scheduler implementation to handle communications.
 *
 * comp-workers produce and give communication work to the scheduler
 * while comm-workers take and consume communication work.
 *
 * IMPL: There is one outbox deque per worker. Communication workers can
 *       steal handlers from other workers outbox.
 */
 void hcCommSchedulerStart(ocrScheduler_t * self, ocrPolicyDomain_t * pd) {
    ocrSchedulerHcCommDelegate_t * commSched = (ocrSchedulerHcCommDelegate_t *) self;
    commSched->baseStart(self, pd);
    //Create outbox queues for each worker
    u64 boxCount = pd->workerCount;
    commSched->outboxesCount = boxCount;
    commSched->outboxes = pd->fcts.pdMalloc(pd, sizeof(deque_t *) * boxCount);
    u64 i;
    for(i = 0; i < boxCount; ++i) {
        commSched->outboxes[i] = newWorkStealingDeque(pd, NULL);
    }
    //Create inbox queues for each worker
    commSched->inboxesCount = boxCount;
    commSched->inboxes = pd->fcts.pdMalloc(pd, sizeof(deque_t *) * boxCount);
    commSched->inboxesLocks = pd->fcts.pdMalloc(pd, sizeof(u32) * boxCount);
    for(i = 0; i < boxCount; ++i) {
        commSched->inboxes[i] = newWorkStealingDeque(pd, NULL);
        commSched->inboxesLocks[i] = 0;
    }
}

void hcCommSchedulerStop(ocrScheduler_t * self) {
    // deallocate all pdMalloc-ed structures
    ocrPolicyDomain_t * pd = self->pd;
    ocrSchedulerHcCommDelegate_t * commSched = (ocrSchedulerHcCommDelegate_t *) self;
    u64 i;
    for(i = 0; i < commSched->outboxesCount; ++i) {
        pd->fcts.pdFree(pd, commSched->outboxes[i]);
    }
    for(i = 0; i < commSched->inboxesCount; ++i) {
        pd->fcts.pdFree(pd, commSched->inboxes[i]);
    }
    pd->fcts.pdFree(pd, commSched->outboxes);
    pd->fcts.pdFree(pd, commSched->inboxes);
    commSched->baseStop(self);
}

/**
 * @brief Take communication work
 *
 * The scheduler must identify the type of the worker doing the call to
 * determine what to return.
 * In the case of comp-worker, a take returns a handle that has completed, while
 * for comm-worker a take returns a handle representing a message to be sent out.
 */
u8 hcCommSchedulerTakeComm(ocrScheduler_t *self, u32* count, ocrFatGuid_t * fatHandlers, u32 properties) {
    ocrSchedulerHcCommDelegate_t * commSched = (ocrSchedulerHcCommDelegate_t *) self;
    ocrWorker_t *worker = NULL; //DIST-TODO sep-concern: Do we need a way to register worker types somehow ?
    getCurrentEnv(NULL, &worker, NULL, NULL);
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;
    u64 wid = hcWorker->id;

    if (hcWorker->hcType == HC_WORKER_COMM) {
        // Steal from other worker's outbox
        //PERF: use real randomized iterator here
        // Try a round of stealing on other worker's outbox
        //NOTE: If we don't care about wasting a steal on self, we wouldn't need
        //      the whole worker ID business
        //NOTE: It's debatable whether we should steal a bunch from each outbox
        //      or go over all outboxes
        deque_t ** outboxes = commSched->outboxes;
        u64 outboxesCount = commSched->outboxesCount;
        u32 success = 0;

#ifdef HYBRID_COMM_COMP_WORKER // Experimental see documentation
        // Try to pop from own outbox first (support HYBRID_COMM_COMP_WORKER mode)
        ocrMsgHandle_t* handle = outboxes[wid]->popFromTail(outboxes[wid], 1);
        if (handle != NULL) {
            fatHandlers[success].metaDataPtr = handle;
            success++;
        }
#endif

        u64 i = (wid+1); // skip over 'self' outbox
        while ((i < (wid+outboxesCount)) && (success < *count)) {
            deque_t * deque = outboxes[i%outboxesCount];
            ocrMsgHandle_t* handle = deque->popFromHead(deque, 1);
            if (handle != NULL) {
                fatHandlers[success].metaDataPtr = handle;
                success++;
            }
            i++;
        }
        *count = success;
    } else {
        //TODO Should really revisit this implementation. It sounds awfully slow.
        ASSERT(hcWorker->hcType == HC_WORKER_COMP);
        deque_t * inbox = commSched->inboxes[wid];
        u32 curIdx = 0;
        linkedlist_t * candidateList = NULL;
        while(curIdx < *count) {
            ocrMsgHandle_t ** target = (ocrMsgHandle_t **) fatHandlers[curIdx].metaDataPtr;
            bool isSpecificTarget = ((target != NULL) && (*target != NULL));
            ocrMsgHandle_t * candidate = NULL;
            if (isSpecificTarget && (candidateList != NULL)) {
                //Look through previous steals
                iterator_t * iterator = candidateList->iterator(candidateList);
                while (iterator->hasNext(iterator)) {
                    ocrMsgHandle_t * handle = (ocrMsgHandle_t *) iterator->next(iterator);
                    if (handle == *target) {
                        // found a candidate in the previous steals
                        candidate = handle;
                        iterator->removeCurrent(iterator);
                        fatHandlers[curIdx].metaDataPtr = candidate;
                        curIdx++;
                        break;
                    }
                }
                iterator->destruct(iterator);
            }
            if (candidate == NULL) {
                // 'steal' from own inbox, comm-worker pushes to it
                candidate = (ocrMsgHandle_t *) inbox->popFromHead(inbox, 0);
                if (candidate == NULL) {
                    // No message available
                    break;
                }
                if (isSpecificTarget) {
                    if (candidate != *target) {
                        if (candidateList == NULL) {
                            ocrPolicyDomain_t * pd;
                            getCurrentEnv(&pd, NULL, NULL, NULL);
                            candidateList = newLinkedList(pd);
                        }
                        candidateList->pushFront(candidateList, candidate);
                    } else {
                        fatHandlers[curIdx].metaDataPtr = candidate;
                        curIdx++;
                    }
                }
            }
        }
        u32 i = curIdx;
        while (i < *count) { // nullify remaining handlers
            fatHandlers[i].metaDataPtr = NULL;
            i++;
        }
        *count = curIdx;
        if (candidateList != NULL) {
            // Put stolen handles for which there's no interest back
            u32 * inboxLock = &commSched->inboxesLocks[wid];
            hal_lock32(inboxLock);
            iterator_t * iterator = candidateList->iterator(candidateList);
            while (iterator->hasNext(iterator)) {
                ocrMsgHandle_t * handle = (ocrMsgHandle_t *) iterator->next(iterator);
                inbox->pushAtTail(inbox, handle, 0);
                iterator->removeCurrent(iterator);
            }
            iterator->destruct(iterator);
            candidateList->destruct(candidateList);
            hal_unlock32(inboxLock);
        }
    }
    return 0;
}

/**
 * @brief Called by comm and comp workers to give communication work.
 *
 * The scheduler must identify the type of the worker doing the call to
 * determine what to do.
 */
u8 hcCommSchedulerGiveComm(ocrScheduler_t *self, u32* count, ocrFatGuid_t* fatHandlers, u32 properties) {
    ocrSchedulerHcCommDelegate_t * commSched = (ocrSchedulerHcCommDelegate_t *) self;
    ocrWorker_t *worker = NULL; //DIST-TODO sep-concern: Do we need a way to register worker types somehow ?
    getCurrentEnv(NULL, &worker, NULL, NULL);
    ocrWorkerHc_t * hcWorker = (ocrWorkerHc_t *) worker;

    if (hcWorker->hcType == HC_WORKER_COMM) {
        u32 i=0;
        while (i < *count) {
            delegateMsgHandle_t* handle = (delegateMsgHandle_t *) fatHandlers[i].metaDataPtr;
        #ifdef HYBRID_COMM_COMP_WORKER // Experimental see documentation
            ocrPolicyMsg_t * message = (handle->handle.status == HDL_RESPONSE_OK) ? handle->handle.response : handle->handle.msg;
            bool outgoingComm = (message->destLocation != self->pd->myLocation);
            if (outgoingComm) {
                // (support HYBRID_COMM_COMP_WORKER mode)
                // The communication worker can process simple messages that
                // are known to be short lived and are 'sterile' (i.e. do not generate
                // new communication) beside responding to the message.
                // In that case, the comm-worker should only be able to give outgoing responses to the scheduler
                ASSERT(message->type & PD_MSG_RESPONSE);
                // Push to the comm worker outbox
                DPRINTF(DEBUG_LVL_VVERB,"[%d] hc-comm-delegate-scheduler:: Comm-worker pushes outgoing to own outbox %d\n",
                    (int) self->pd->myLocation, hcWorker->id);
                deque_t * outbox = commSched->outboxes[hcWorker->id];
                outbox->pushAtTail(outbox, handle, 0);
            } else {
        #endif
                // Comm-worker giving back to a worker's inbox
                DPRINTF(DEBUG_LVL_VVERB,"[%d] hc-comm-delegate-scheduler:: Comm-worker pushes at tail of box %d\n",
                    (int) self->pd->myLocation, handle->boxId);
                // The lock is required because the comp-worker can push back handles it took
                u32 * inboxLock = &commSched->inboxesLocks[handle->boxId];
                hal_lock32(inboxLock);
                deque_t * inbox = commSched->inboxes[handle->boxId];
                inbox->pushAtTail(inbox, (ocrMsgHandle_t *) handle, 0);
                hal_unlock32(inboxLock);
        #ifdef HYBRID_COMM_COMP_WORKER
            }
        #endif
            i++;
        }
        *count = i;
    } else {
        // Comp-worker giving a message to send
        u32 i=0;
        while (i < *count) {
            // Set delegate handle's box id.
            delegateMsgHandle_t* delHandle = (delegateMsgHandle_t *) fatHandlers[i].metaDataPtr;
            //DIST-TODO: boxId is defined in del-handle however only the scheduler is using it
            delHandle->boxId = hcWorker->id;
            DPRINTF(DEBUG_LVL_VVERB,"[%d] hc-comm-delegate-scheduler:: Comp-worker pushes at tail of box %d\n",
                (int) self->pd->myLocation, delHandle->boxId);
            ASSERT((delHandle->boxId >= 0) && (delHandle->boxId < self->pd->workerCount));
            // Put handle to worker's outbox
            deque_t * outbox = commSched->outboxes[delHandle->boxId];
            outbox->pushAtTail(outbox, (ocrMsgHandle_t *) delHandle, 0);
            i++;
        }
        *count = i;
    }
    return 0;
}

u8 hcCommMonitorProgress(ocrScheduler_t *self, ocrMonitorProgress_t type, void * monitoree) {
    ocrSchedulerHcCommDelegate_t * commSched = (ocrSchedulerHcCommDelegate_t *) self;
    return commSched->baseMonitorProgress(self, type, monitoree);
}

ocrScheduler_t* newSchedulerHcComm(ocrSchedulerFactory_t * factory, ocrParamList_t *perInstance) {
    ocrScheduler_t* base = (ocrScheduler_t*) runtimeChunkAlloc(sizeof(ocrSchedulerHcCommDelegate_t), NULL);
    factory->initialize(factory, base, perInstance);
    return base;
}

void initializeSchedulerHcComm(ocrSchedulerFactory_t * factory, ocrScheduler_t *self, ocrParamList_t *perInstance) {
    ocrSchedulerFactoryHcComm_t * derivedFactory = (ocrSchedulerFactoryHcComm_t *) factory;
    // Initialize the base scheduler
    derivedFactory->baseInitialize(factory, self, perInstance);
    ocrSchedulerHcCommDelegate_t * commSched = (ocrSchedulerHcCommDelegate_t *) self;
    commSched->outboxesCount = 0;
    commSched->outboxes = NULL;
    commSched->inboxesCount = 0;
    commSched->inboxes = NULL;
    commSched->baseStart = derivedFactory->baseStart;
    commSched->baseStop = derivedFactory->baseStop;
    commSched->baseMonitorProgress = derivedFactory->baseMonitorProgress;
}

void destructSchedulerFactoryHcComm(ocrSchedulerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrSchedulerFactory_t * newOcrSchedulerFactoryHcCommDelegate(ocrParamList_t *perType) {
    // Create a base factory to get a hold at parent's impl function pointers.
    ocrSchedulerFactory_t * baseFactory = newOcrSchedulerFactoryHc(perType);
    ocrSchedulerFcts_t baseFcts = baseFactory->schedulerFcts;
    ocrSchedulerFactoryHcComm_t* derived = (ocrSchedulerFactoryHcComm_t*) runtimeChunkAlloc(
        sizeof(ocrSchedulerFactoryHcComm_t), (void *)1);

    // Store function pointers we need from the base implementation
    derived->baseInitialize = baseFactory->initialize;
    derived->baseStart = baseFcts.start;
    derived->baseStop = baseFcts.stop;
    derived->baseMonitorProgress = baseFcts.monitorProgress;

    ocrSchedulerFactory_t* base = (ocrSchedulerFactory_t*) derived;
    base->instantiate = FUNC_ADDR(ocrScheduler_t* (*)(ocrSchedulerFactory_t*, ocrParamList_t*), newSchedulerHcComm);
    base->initialize = FUNC_ADDR(void (*)(ocrSchedulerFactory_t*, ocrScheduler_t*, ocrParamList_t*), initializeSchedulerHcComm);
    base->destruct = FUNC_ADDR(void (*)(ocrSchedulerFactory_t*), destructSchedulerFactoryHcComm);
    // Copy base's function pointers
    base->schedulerFcts = baseFcts;
    // and specialize some
    base->schedulerFcts.start = FUNC_ADDR(void (*)(ocrScheduler_t*, ocrPolicyDomain_t*), hcCommSchedulerStart);
    base->schedulerFcts.stop  = FUNC_ADDR(void (*)(ocrScheduler_t*), hcCommSchedulerStop);
    base->schedulerFcts.takeComm = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*, u32), hcCommSchedulerTakeComm);
    base->schedulerFcts.giveComm = FUNC_ADDR(u8 (*)(ocrScheduler_t*, u32*, ocrFatGuid_t*, u32), hcCommSchedulerGiveComm);
    base->schedulerFcts.monitorProgress = FUNC_ADDR(u8 (*)(ocrScheduler_t*, ocrMonitorProgress_t, void*), hcCommMonitorProgress);
    baseFactory->destruct(baseFactory);
    return base;
}

#endif /* ENABLE_SCHEDULER_HC_COMM_DELEGATE */
