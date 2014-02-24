/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_WORKER_HC_COMM

#include "debug.h"
#include "ocr-db.h"
#include "ocr-policy-domain.h"
#include "ocr-sysboot.h"
#include "ocr-worker.h"
#include "worker/hc/hc-worker.h"
#include "worker/hc-comm/hc-comm-worker.h"
#include "ocr-errors.h"

#include "experimental/ocr-placer.h"
#include "extensions/ocr-affinity.h"

#define DEBUG_TYPE WORKER

/******************************************************/
/* OCR-HC COMMUNICATION WORKER                        */
/* Extends regular HC workers                         */
/******************************************************/

ocrGuid_t processRequestEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    ocrPolicyMsg_t * msg = (ocrPolicyMsg_t *) paramv[0];
    ocrPolicyDomain_t * pd;
    getCurrentEnv(&pd, NULL, NULL, NULL);
    // This is meant to execute incoming request and asynchronously processed responses (two-way asynchronous)
    // Regular responses are routed back to requesters by the scheduler and are processed by them.
    ASSERT((msg->type & PD_MSG_REQUEST) ||
        ((msg->type & PD_MSG_RESPONSE) && ((msg->type & PD_MSG_TYPE_ONLY) == PD_MSG_DB_ACQUIRE)));
    // Important to read this before calling processMessage. If the message requires
    // a response, the runtime reuses the request's message to post the response.
    // Hence there's a race between this code and the code posting the response.
    bool processResponse = !!(msg->type & PD_MSG_RESPONSE); // mainly for debug
    // DB_ACQUIRE are potentially asynchronous
    bool syncProcess = ((msg->type & PD_MSG_TYPE_ONLY) != PD_MSG_DB_ACQUIRE);
    bool toBeFreed = !(msg->type & PD_MSG_REQ_RESPONSE);
    DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: Process incoming EDT request @ %p of type 0x%x\n", msg, msg->type);
    u8 res = pd->fcts.processMessage(pd, msg, syncProcess);
    DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: [done] Process incoming EDT @ %p request of type 0x%x\n", msg, msg->type);
    // Either flagged or was an asynchronous processing, the implementation should
    // have setup a callback and we can free the request message
    if (toBeFreed || (!syncProcess && (res == OCR_EPEND))) {
        // Makes sure the runtime doesn't try to reuse this message
        // even though it was not supposed to issue a response.
        // If that's the case, this check is racy
        ASSERT(!(msg->type & PD_MSG_RESPONSE) || processResponse);
        DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: Deleted incoming EDT request @ %p of type 0x%x\n", msg, msg->type);
        // if request was an incoming one-way we can delete the message now.
        pd->fcts.pdFree(pd, msg);
    }

    return NULL_GUID;
}

static u8 takeFromSchedulerAndSend(ocrPolicyDomain_t * pd) {
    // When the communication-worker is not stopping only a single iteration is
    // executed. Otherwise it is executed until the scheduler's 'take' do not
    // return any more work.
    ocrMsgHandle_t * outgoingHandle = NULL;
    PD_MSG_STACK(msgCommTake);
    u8 ret = 0;
    getCurrentEnv(NULL, NULL, NULL, &msgCommTake);
    ocrFatGuid_t handlerGuid = {.guid = NULL_GUID, .metaDataPtr = NULL};
    //IMPL: MSG_COMM_TAKE implementation must be consistent across PD, Scheduler and Worker.
    // We expect the PD to fill-in the guids pointer with an ocrMsgHandle_t pointer
    // to be processed by the communication worker or NULL.
    //PERF: could request 'n' for internal comm load balancing (outgoing vs pending vs incoming).
    #define PD_MSG (&msgCommTake)
    #define PD_TYPE PD_MSG_COMM_TAKE
    msgCommTake.type = PD_MSG_COMM_TAKE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guids) = &handlerGuid;
    PD_MSG_FIELD_IO(extra) = 0; /*unused*/
    PD_MSG_FIELD_IO(guidCount) = 1;
    PD_MSG_FIELD_I(properties) = 0;
    PD_MSG_FIELD_IO(type) = OCR_GUID_COMM;
    ret = pd->fcts.processMessage(pd, &msgCommTake, true);
    if (!ret && (PD_MSG_FIELD_IO(guidCount) != 0)) {
        ASSERT(PD_MSG_FIELD_IO(guidCount) == 1); //LIMITATION: single guid returned by comm take
        ocrFatGuid_t handlerGuid = PD_MSG_FIELD_IO(guids[0]);
        ASSERT(handlerGuid.metaDataPtr != NULL);
        outgoingHandle = (ocrMsgHandle_t *) handlerGuid.metaDataPtr;
    #undef PD_MSG
    #undef PD_TYPE
        if (outgoingHandle != NULL) {
            // This code handles the pd's outgoing messages. They can be requests or responses.
            DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: outgoing handle comm take successful handle=%p, msg=%p type=0x%x\n",
                    outgoingHandle, outgoingHandle->msg, outgoingHandle->msg->type);
            //We can never have an outgoing handle with the response ptr set because
            //when we process an incoming request, we lose the handle by calling the
            //pd's process message. Hence, a new handle is created for one-way response.
            ASSERT(outgoingHandle->response == NULL);
            u32 properties = outgoingHandle->properties;
            ASSERT(properties & PERSIST_MSG_PROP);
            //DIST-TODO design: Not sure where to draw the line between one-way with/out ack implementation
            //If the worker was not aware of the no-ack policy, is it ok to always give a handle
            //and the comm-api contract is to at least set the HDL_SEND_OK flag ?
            ocrMsgHandle_t ** sendHandle = ((properties & TWOWAY_MSG_PROP) && !(properties & ASYNC_MSG_PROP))
                ? &outgoingHandle : NULL;
            //DIST-TODO design: who's responsible for deallocating the handle ?
            //If the message is two-way, the creator of the handle is responsible for deallocation
            //If one-way, the comm-layer disposes of the handle when it is not needed anymore
            //=> Sounds like if an ack is expected, caller is responsible for dealloc, else callee
            pd->fcts.sendMessage(pd, outgoingHandle->msg->destLocation, outgoingHandle->msg, sendHandle, properties);

            // This is contractual for now. It recycles the handler allocated in the delegate-comm-api:
            // - Sending a request one-way or a response (always non-blocking): The delegate-comm-api
            //   creates the handle merely to be able to give it to the scheduler. There's no use of the
            //   handle beyond this point.
            // - The runtime does not implement blocking one-way. Hence, the callsite of the original
            //   send message did not ask for a handler to be returned.
            if (sendHandle == NULL) {
                outgoingHandle->destruct(outgoingHandle);
            }

            //Communication is posted. If TWOWAY, subsequent calls to poll may return the response
            //to be processed
            return POLL_MORE_MESSAGE;
        }
    }
    return POLL_NO_MESSAGE;
}

static void workerLoopHcComm_RL2(ocrWorker_t * worker) {
    ocrWorkerHcComm_t * self = (ocrWorkerHcComm_t *) worker;
    ocrPolicyDomain_t *pd = worker->pd;

    // Take and send all the work
    while(takeFromSchedulerAndSend(pd) == POLL_MORE_MESSAGE);

    // Poll for completion of all outstanding
    ocrMsgHandle_t * handle = NULL;
    pd->fcts.pollMessage(pd, &handle);
    // DIST-TODO stop: this is borderline, because we've transitioned
    // the comm-platform to RL2, we assume that poll blocks until
    // all messages have been processed.
    self->rl_completed[2] = true;
}

static u8 createProcessRequestEdt(ocrPolicyDomain_t * pd, ocrGuid_t templateGuid, u64 * paramv) {

    ocrGuid_t edtGuid;
    u32 paramc = 1;
    u32 depc = 0;
    u32 properties = 0;
    ocrWorkType_t workType = EDT_RT_WORKTYPE;

    START_PROFILE(api_EdtCreate);
    PD_MSG_STACK(msg);
    u8 returnCode = 0;
    ocrTask_t *curEdt = NULL;
    getCurrentEnv(NULL, NULL, &curEdt, &msg);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_WORK_CREATE
    msg.type = PD_MSG_WORK_CREATE | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD_IO(guid.guid) = NULL_GUID;
    PD_MSG_FIELD_IO(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD_I(templateGuid.guid) = templateGuid;
    PD_MSG_FIELD_I(templateGuid.metaDataPtr) = NULL;
    PD_MSG_FIELD_I(affinity.guid) = NULL_GUID;
    PD_MSG_FIELD_I(affinity.metaDataPtr) = NULL;
    PD_MSG_FIELD_IO(outputEvent.guid) = NULL_GUID;
    PD_MSG_FIELD_IO(outputEvent.metaDataPtr) = NULL;
    PD_MSG_FIELD_I(paramv) = paramv;
    PD_MSG_FIELD_IO(paramc) = paramc;
    PD_MSG_FIELD_IO(depc) = depc;
    PD_MSG_FIELD_I(depv) = NULL;
    PD_MSG_FIELD_I(properties) = properties;
    PD_MSG_FIELD_I(workType) = workType;
    // This is a "fake" EDT so it has no "parent"
    PD_MSG_FIELD_I(currentEdt.guid) = NULL_GUID;
    PD_MSG_FIELD_I(currentEdt.metaDataPtr) = NULL;
    PD_MSG_FIELD_I(parentLatch.guid) = NULL_GUID;
    PD_MSG_FIELD_I(parentLatch.metaDataPtr) = NULL;
    returnCode = pd->fcts.processMessage(pd, &msg, true);
    if(returnCode) {
        edtGuid = PD_MSG_FIELD_IO(guid.guid);
        DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: Created processRequest EDT GUID 0x%lx\n", edtGuid);
        RETURN_PROFILE(returnCode);
    }

    RETURN_PROFILE(0);
#undef PD_MSG
#undef PD_TYPE
}

static void workerLoopHcComm_RL3(ocrWorker_t * worker) {
    ocrWorkerHcComm_t * self = (ocrWorkerHcComm_t *) worker;
    ocrPolicyDomain_t *pd = worker->pd;

    ocrGuid_t processRequestTemplate = NULL_GUID;
    ocrEdtTemplateCreate(&processRequestTemplate, &processRequestEdt, 1, 0);

    // This loop exits on the first call to stop.
    while(!self->rl_completed[3]) {
        // First, Ask the scheduler if there are any communication
        // to be scheduled and sent them if any.
        takeFromSchedulerAndSend(pd);

        ocrMsgHandle_t * handle = NULL;
        u8 ret = pd->fcts.pollMessage(pd, &handle);
        if (ret == POLL_MORE_MESSAGE) {
            //IMPL: for now only support successful polls on incoming request and responses
            ASSERT((handle->status == HDL_RESPONSE_OK)||(handle->status == HDL_NORMAL));
            ocrPolicyMsg_t * message = (handle->status == HDL_RESPONSE_OK) ? handle->response : handle->msg;
            //To catch misuses, assert src is not self and dst is self
            ASSERT((message->srcLocation != pd->myLocation) && (message->destLocation == pd->myLocation));
            // Poll a response to a message we had sent.
            if ((message->type & PD_MSG_RESPONSE) && !(handle->properties & ASYNC_MSG_PROP)) {
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: Received message response for msgId: %ld\n",  message->msgId); // debug
                // Someone is expecting this response, give it back to the PD
                ocrFatGuid_t fatGuid;
                fatGuid.metaDataPtr = handle;
                PD_MSG_STACK(giveMsg);
                getCurrentEnv(NULL, NULL, NULL, &giveMsg);
            #define PD_MSG (&giveMsg)
            #define PD_TYPE PD_MSG_COMM_GIVE
                giveMsg.type = PD_MSG_COMM_GIVE | PD_MSG_REQUEST;
                PD_MSG_FIELD_IO(guids) = &fatGuid;
                PD_MSG_FIELD_IO(guidCount) = 1;
                PD_MSG_FIELD_I(properties) = 0;
                PD_MSG_FIELD_I(type) = OCR_GUID_COMM;
                ret = pd->fcts.processMessage(pd, &giveMsg, false);
                ASSERT(ret == 0);
            #undef PD_MSG
            #undef PD_TYPE
                //For now, assumes all the responses are for workers that are
                //waiting on the response handler provided by sendMessage, reusing
                //the request msg as an input buffer for the response.
            } else {
                ASSERT((message->type & PD_MSG_REQUEST) || ((message->type & PD_MSG_RESPONSE) && (handle->properties & ASYNC_MSG_PROP)));
                // else it's a request or a response with ASYNC_MSG_PROP set (i.e. two-way but asynchronous handling of response).
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: Received message, msgId: %ld type:0x%x prop:0x%x\n",
                                        message->msgId, message->type, handle->properties);
                // This is an outstanding request, delegate to PD for processing
                u64 msgParamv = (u64) message;
            #ifdef HYBRID_COMM_COMP_WORKER // Experimental see documentation
                // Execute selected 'sterile' messages on the spot
                if ((message->type & PD_MSG_TYPE_ONLY) == PD_MSG_DB_ACQUIRE) {
                    DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: Execute message, msgId: %ld\n", pd->myLocation, message->msgId);
                    processRequestEdt(1, &msgParamv, 0, NULL);
                } else {
                    createProcessRequestEdt(pd, processRequestTemplate, &msgParamv);
                }
            #else
                createProcessRequestEdt(pd, processRequestTemplate, &msgParamv);
            #endif
                // We do not need the handle anymore
                handle->destruct(handle);
                //DIST-TODO-3: depending on comm-worker implementation, the received message could
                //then be 'wrapped' in an EDT and pushed to the deque for load-balancing purpose.
            }

        } else {
            //DIST-TODO No messages ready for processing, ask PD for EDT work.
        }
    } // run-loop
}

static void workerLoopHcComm(ocrWorker_t * worker) {
    ocrWorkerHcComm_t * self = (ocrWorkerHcComm_t *) worker;

    // RL3: take, send / poll, dispatch, execute
    workerLoopHcComm_RL3(worker);
    ASSERT(self->rl_completed[3]);
    self->rl_completed[3] = false;
    while(self->rl == 3); // transitioned by 'stop'

    // RL2: Empty the scheduler of all communication work,
    // then keep polling so that the comm-api can drain messages
    workerLoopHcComm_RL2(worker);
    while(self->rl == 2); // transitioned by 'stop'

    // RL1: Wait for PD stop.
    while(worker->fcts.isRunning(worker));
}

static bool isBlessedWorker(ocrWorker_t * worker, ocrGuid_t * affinityMasterPD) {
    // Determine if current worker is the master worker of this PD
    bool blessedWorker = (worker->type == MASTER_WORKERTYPE);
    if (blessedWorker) {
        // Determine if current master worker is part of master PD
        u64 count = 0;
        // There should be a single master PD
        ASSERT(!ocrAffinityCount(AFFINITY_PD_MASTER, &count) && (count == 1));
        ocrAffinityGet(AFFINITY_PD_MASTER, &count, affinityMasterPD);
        ASSERT(count == 1);
        blessedWorker &= (worker->pd->myLocation == affinityToLocation(*affinityMasterPD));
    }
    return blessedWorker;
}

void* runWorkerHcComm(ocrWorker_t * worker) {
    ocrGuid_t affinityMasterPD;
    bool blessedWorker = isBlessedWorker(worker, &affinityMasterPD);
    DPRINTF(DEBUG_LVL_INFO,"hc-comm-worker: blessed worker at location %d\n",
            (int)worker->pd->myLocation);
    if (blessedWorker) {
        // This is all part of the mainEdt setup
        // and should be executed by the "blessed" worker.
        void * packedUserArgv = userArgsGet();
        ocrEdt_t mainEdt = mainEdtGet();

        u64 totalLength = ((u64*) packedUserArgv)[0]; // already exclude this first arg
        // strip off the 'totalLength first argument'
        packedUserArgv = (void *) (((u64)packedUserArgv) + sizeof(u64)); // skip first totalLength argument
        ocrGuid_t dbGuid;
        void* dbPtr;
        ocrDbCreate(&dbGuid, &dbPtr, totalLength,
                    DB_PROP_IGNORE_WARN, affinityMasterPD, NO_ALLOC);
        DPRINTF(DEBUG_LVL_INFO,"mainDb guid 0x%lx ptr %p\n", dbGuid, dbPtr);
        // copy packed args to DB
        hal_memCopy(dbPtr, packedUserArgv, totalLength, 0);

        // Prepare the mainEdt for scheduling
        ocrGuid_t edtTemplateGuid = NULL_GUID, edtGuid = NULL_GUID;
        ocrEdtTemplateCreate(&edtTemplateGuid, mainEdt, 0, 1);
        ocrEdtCreate(&edtGuid, edtTemplateGuid, EDT_PARAM_DEF, /* paramv = */ NULL,
                    /* depc = */ EDT_PARAM_DEF, /* depv = */ &dbGuid,
                    EDT_PROP_NONE, affinityMasterPD, NULL);
    } else {
        // Set who we are
        ocrPolicyDomain_t *pd = worker->pd;
        u32 i;
        for(i = 0; i < worker->computeCount; ++i) {
            worker->computes[i]->fcts.setCurrentEnv(worker->computes[i], pd, worker);
        }
    }

    DPRINTF(DEBUG_LVL_INFO, "Starting scheduler routine of worker %ld\n", ((ocrWorkerHc_t *) worker)->id);
    workerLoopHcComm(worker);
    return NULL;
}


void stopWorkerHcComm(ocrWorker_t * selfBase) {
    ocrWorkerHcComm_t * self = (ocrWorkerHcComm_t *) selfBase;
    ocrWorker_t * currWorker = NULL;
    getCurrentEnv(NULL, &currWorker, NULL, NULL);

    if (self->rl_completed[self->rl]) {
        self->rl--;
    }
    // Some other worker wants to stop the communication worker.
    if (currWorker != selfBase) {
        switch(self->rl) {
            case 3:
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: begin shutdown RL3\n");
                // notify worker of level completion
                self->rl_completed[3] = true;
                while(self->rl_completed[3]);
                self->rl_completed[3] = true; // so ugly
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: done shutdown RL3\n");
                // Guarantees RL3 is fully completed. Avoids race where the RL is flipped
                // but the comm-worker is in the middle of its RL3 loop.
            break;
            case 2:
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: begin shutdown RL2\n");
                // Wait for runlevel to complete
                while(!self->rl_completed[2]);
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: done shutdown RL2\n");
                // All communications completed.
            break;
            case 1:
                //DIST-TODO stop: we don't need this RL I think
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: begin shutdown RL1\n");
                self->rl_completed[1] = true;
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: done shutdown RL1\n");
            break;
            case 0:
                DPRINTF(DEBUG_LVL_VVERB,"hc-comm-worker: begin shutdown RL0\n");
                // We go on and call the base stop function that
                // shuts down the comm-api and comm-platform.
                self->baseStop(selfBase);
            break;
            default:
            ASSERT(false && "hc-comm-worker: Illegal runlevel in shutdown");
        }
    }  else {
        //DIST-TODO stop: Implement self shutdown
        ASSERT(false && "hc-comm-worker: Implement self shutdown");
        // worker stopping itself, just call the appropriate run-level
    }
}

/**
 * Builds an instance of a HC Communication worker
 */
ocrWorker_t* newWorkerHcComm(ocrWorkerFactory_t * factory, ocrParamList_t * perInstance) {
    ocrWorker_t * worker = (ocrWorker_t*)
            runtimeChunkAlloc(sizeof(ocrWorkerHcComm_t), NULL);
    factory->initialize(factory, worker, perInstance);
    return (ocrWorker_t *) worker;
}

void initializeWorkerHcComm(ocrWorkerFactory_t * factory, ocrWorker_t *self, ocrParamList_t *perInstance) {
    ocrWorkerFactoryHcComm_t * derivedFactory = (ocrWorkerFactoryHcComm_t *) factory;
    derivedFactory->baseInitialize(factory, self, perInstance);
    // Override base's default value
    ocrWorkerHc_t * workerHc = (ocrWorkerHc_t *) self;
    workerHc->hcType = HC_WORKER_COMM;
    // Initialize comm=worker's members
    ocrWorkerHcComm_t * workerHcComm = (ocrWorkerHcComm_t *) self;
    workerHcComm->baseStop = derivedFactory->baseStop;
    int i = 0;
    while (i < (HC_COMM_WORKER_RL_MAX+1)) {
        workerHcComm->rl_completed[i++] = false;
    }
    workerHcComm->rl = HC_COMM_WORKER_RL_MAX;
}

/******************************************************/
/* OCR-HC COMMUNICATION WORKER FACTORY                */
/******************************************************/

void destructWorkerFactoryHcComm(ocrWorkerFactory_t * factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrWorkerFactory_t * newOcrWorkerFactoryHcComm(ocrParamList_t * perType) {
    ocrWorkerFactory_t * baseFactory = newOcrWorkerFactoryHc(perType);
    ocrWorkerFcts_t baseFcts = baseFactory->workerFcts;

    ocrWorkerFactoryHcComm_t* derived = (ocrWorkerFactoryHcComm_t*)runtimeChunkAlloc(sizeof(ocrWorkerFactoryHcComm_t), (void *)1);
    ocrWorkerFactory_t * base = (ocrWorkerFactory_t *) derived;
    base->instantiate = FUNC_ADDR(ocrWorker_t* (*)(ocrWorkerFactory_t*, ocrParamList_t*), newWorkerHcComm);
    base->initialize  = FUNC_ADDR(void (*)(ocrWorkerFactory_t*, ocrWorker_t*, ocrParamList_t*), initializeWorkerHcComm);
    base->destruct    = FUNC_ADDR(void (*)(ocrWorkerFactory_t*), destructWorkerFactoryHcComm);

    // Copy base's function pointers
    base->workerFcts = baseFcts;
    derived->baseInitialize = baseFactory->initialize;
    derived->baseStop = baseFcts.stop;

    // Specialize comm functions
    base->workerFcts.run = FUNC_ADDR(void* (*)(ocrWorker_t*), runWorkerHcComm);
    base->workerFcts.workShift = FUNC_ADDR(void* (*) (ocrWorker_t *), workerLoopHcComm_RL3);
    base->workerFcts.stop = FUNC_ADDR(void (*)(ocrWorker_t*), stopWorkerHcComm);

    baseFactory->destruct(baseFactory);
    return base;
}

#endif /* ENABLE_WORKER_HC_COMM */
