/**
 * @brief OCR implementation of statistics callbacks
 **/

/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#ifdef OCR_ENABLE_STATISTICS

#include "debug.h"
#include "ocr-statistics.h"
#include "ocr-statistics-callbacks.h"
#include "ocr-types.h"
#include "ocr-task.h"
#include "ocr-datablock.h"
#include "ocr-event.h"
#include "ocr-allocator.h"
#include "ocr-worker.h"
#include "ocr-policy-domain.h"


#define DEBUG_TYPE STATS

void statsTEMP_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                          ocrGuid_t templateGuid, ocrTaskTemplate_t *ttemplate) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!ttemplate) {
        deguidify(pd, templateGuid, (u64*)&ttemplate, NULL);
    }
    ttemplate->statProcess = stats->fctPtrs->createStatsProcess(stats, templateGuid);    
    if(edtGuid == NULL_GUID) {
        // This is the first creation. No parent EDT
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(task->guid == edtGuid);
    ASSERT(ttemplate->guid == templateGuid);
    
    ocrStatsProcess_t *srcProcess = task->statProcess;
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_TEMP_CREATE, edtGuid, templateGuid,
        NULL);
        
    ocrStatsAsyncMessage(srcProcess, ttemplate->statProcess, mess);    
}

void statsTEMP_USE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                   ocrGuid_t templateGuid, ocrTaskTemplate_t *ttemplate) {
    
    ocrStats_t *stats = pd->getStats(pd);

    if(edtGuid == NULL_GUID)
        return;
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    if(!ttemplate) {
        deguidify(pd, templateGuid, (u64*)&ttemplate, NULL);
    }

    ASSERT(task->guid == edtGuid);
    ASSERT(ttemplate->guid == templateGuid);

    ocrStatsProcess_t *srcProcess = task->statProcess;
    
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_TEMP_USE, edtGuid, templateGuid, NULL);
    ocrStatsSyncMessage(srcProcess, ttemplate->statProcess, mess);
}

void statsTEMP_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                       ocrGuid_t templateGuid, ocrTaskTemplate_t *ttemplate) {

    ocrStats_t *stats = pd->getStats(pd);
    
    if(!ttemplate) {
        deguidify(pd, templateGuid, (u64*)&ttemplate, NULL);
    }
    
    if(edtGuid == NULL_GUID) {
        stats->fctPtrs->destructStatsProcess(stats, ttemplate->statProcess);
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(task->guid == edtGuid);
    ASSERT(ttemplate->guid == templateGuid);
    
    ocrStatsProcess_t *srcProcess = task->statProcess;
                    
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_TEMP_DESTROY, edtGuid, templateGuid,
        NULL);
        
    ocrStatsAsyncMessage(srcProcess, ttemplate->statProcess, mess);
    stats->fctPtrs->destructStatsProcess(stats, ttemplate->statProcess);
}

void statsEDT_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                     ocrGuid_t createGuid, ocrTask_t *createTask) {

    ocrStats_t *stats = pd->getStats(pd);

    if(!createTask) {
        deguidify(pd, createGuid, (u64*)&createTask, NULL);
    }
    createTask->statProcess = stats->fctPtrs->createStatsProcess(stats, createGuid);
    
    if(edtGuid == NULL_GUID) {
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(task->guid == edtGuid);
    ASSERT(createTask->guid == createGuid);
    ASSERT(createGuid != edtGuid);

    ocrStatsProcess_t *srcProcess = task->statProcess;

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_EDT_CREATE, edtGuid, createGuid, NULL);
    ocrStatsAsyncMessage(srcProcess, createTask->statProcess, mess);
}

void statsEDT_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                      ocrGuid_t destroyGuid, ocrTask_t *destroyTask) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!destroyTask) {
        deguidify(pd, destroyGuid, (u64*)&destroyTask, NULL);
    }
    
    if(edtGuid == NULL_GUID) {
        stats->fctPtrs->destructStatsProcess(stats, destroyTask->statProcess);
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(task->guid == edtGuid);
    ASSERT(destroyTask->guid == destroyGuid);
    
    ocrStatsProcess_t *srcProcess = task->statProcess;

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_EDT_DESTROY, edtGuid, destroyGuid, NULL);
    ocrStatsAsyncMessage(srcProcess, destroyTask->statProcess, mess);
    stats->fctPtrs->destructStatsProcess(stats, destroyTask->statProcess);
}

void statsEDT_START(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                    ocrGuid_t edtGuid, ocrTask_t *task) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!worker) {
        deguidify(pd, workerGuid, (u64*)&worker, NULL);
    }
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(worker->guid == workerGuid);
    ASSERT(task->guid == edtGuid);

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_EDT_START, workerGuid, edtGuid, NULL);
    ocrStatsSyncMessage(worker->statProcess, task->statProcess, mess);
}

void statsEDT_END(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                  ocrGuid_t edtGuid, ocrTask_t *task) {
    
    ocrStats_t *stats = pd->getStats(pd);
    if(!worker) {
        deguidify(pd, workerGuid, (u64*)&worker, NULL);
    }
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(worker->guid == workerGuid);
    ASSERT(task->guid == edtGuid);

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(
        stats, STATS_EDT_END, workerGuid, edtGuid, NULL);
    ocrStatsSyncMessage(worker->statProcess, task->statProcess, mess);
}

void statsDB_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                    ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                    ocrGuid_t dbGuid, ocrDataBlock_t *db) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!db) {
        deguidify(pd, dbGuid, (u64*)&db, NULL);
    }
    db->statProcess = stats->fctPtrs->createStatsProcess(stats, dbGuid);
    
    if(edtGuid == NULL_GUID) {
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    if(!allocator) {
        deguidify(pd, allocatorGuid, (u64*)&allocator, NULL);
    }

    ASSERT(edtGuid == task->guid);
    ASSERT(allocatorGuid == allocator->guid);
    ASSERT(dbGuid == db->guid);
    ASSERT(db->allocator == allocatorGuid);

    statsParamDb_t params = { .offset = 0, .size = db->size, .isWrite = 1 };

    // TODO: Check if this is the most useful way to message things...
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DB_CREATE, edtGuid, allocatorGuid,
                                                            (ocrStatsParam_t*)&params);
    ocrStatsSyncMessage(task->statProcess, allocator->statProcess, mess);
    mess = stats->fctPtrs->createMessage(stats, STATS_DB_CREATE, allocatorGuid, dbGuid, (ocrStatsParam_t*)&params);
    ocrStatsSyncMessage(allocator->statProcess, db->statProcess, mess);
}

void statsDB_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                     ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                     ocrGuid_t dbGuid, ocrDataBlock_t *db) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!db) {
        deguidify(pd, dbGuid, (u64*)&db, NULL);
    }
    
    if(edtGuid == NULL_GUID) {
        stats->fctPtrs->destructStatsProcess(stats, db->statProcess);
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    if(!allocator) {
        deguidify(pd, allocatorGuid, (u64*)&allocator, NULL);
    }

    ASSERT(edtGuid == task->guid);
    ASSERT(allocatorGuid == allocator->guid);
    ASSERT(dbGuid == db->guid);
    ASSERT(db->allocator == allocatorGuid);

    statsParamDb_t params = { .offset = 0, .size = db->size, .isWrite = 1 };
    // TODO: Check if this is the most useful way to message things...
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DB_DESTROY, edtGuid, allocatorGuid,
                                                            (ocrStatsParam_t*)&params);
    ocrStatsSyncMessage(task->statProcess, allocator->statProcess, mess);
    mess = stats->fctPtrs->createMessage(stats, STATS_DB_DESTROY, allocatorGuid, dbGuid, (ocrStatsParam_t*)&params);
    ocrStatsSyncMessage(allocator->statProcess, db->statProcess, mess);
    
    stats->fctPtrs->destructStatsProcess(stats, db->statProcess);
}

void statsDB_ACQ(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                 ocrGuid_t dbGuid, ocrDataBlock_t *db) {
    ocrStats_t *stats = pd->getStats(pd);
    if(!db) {
        deguidify(pd, dbGuid, (u64*)&db, NULL);
    }
    
    if(edtGuid == NULL_GUID) {
        return;
    }
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(edtGuid == task->guid);
    ASSERT(dbGuid == db->guid);

    statsParamDb_t params = { .offset = 0, .size = db->size, .isWrite = 1 };
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DB_ACQ, edtGuid, dbGuid,
                                                            (ocrStatsParam_t*)&params);
    ocrStatsSyncMessage(task->statProcess, db->statProcess, mess);
}

void statsDB_REL(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                 ocrGuid_t dbGuid, ocrDataBlock_t *db) {
    ocrStats_t *stats = pd->getStats(pd);
    if(!db) {
        deguidify(pd, dbGuid, (u64*)&db, NULL);
    }
    
    if(edtGuid == NULL_GUID) {
        return;
    }
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }

    ASSERT(edtGuid == task->guid);
    ASSERT(dbGuid == db->guid);

    statsParamDb_t params = { .offset = 0, .size = db->size, .isWrite = 1 };
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DB_REL, edtGuid, dbGuid,
                                                            (ocrStatsParam_t*)&params);
    ocrStatsAsyncMessage(task->statProcess, db->statProcess, mess);
}

void statsDB_ACCESS(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                    ocrGuid_t dbGuid, ocrDataBlock_t *db,
                    u64 offset, u64 size, u8 isWrite) {

    ocrStats_t *stats = pd->getStats(pd);
    ASSERT(edtGuid);
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    if(!db) {
        deguidify(pd, dbGuid, (u64*)&db, NULL);
    }

    ASSERT(edtGuid == task->guid);
    ASSERT(dbGuid == db->guid);

    statsParamDb_t params = { .offset = offset, .size = size, .isWrite = isWrite };
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DB_ACCESS, edtGuid, dbGuid,
                                                            (ocrStatsParam_t*)&params);
    if(isWrite)
        ocrStatsAsyncMessage(task->statProcess, db->statProcess, mess);
    else
        ocrStatsSyncMessage(task->statProcess, db->statProcess, mess);
}

void statsEVT_CREATE(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                     ocrGuid_t evtGuid, ocrEvent_t *evt) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!evt) {
        deguidify(pd, evtGuid, (u64*)&evt, NULL);
    }
    evt->statProcess = stats->fctPtrs->createStatsProcess(stats, evtGuid);

    if(edtGuid == NULL_GUID) {
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_EVT_CREATE, edtGuid,
                                                            evtGuid, NULL);
    ocrStatsSyncMessage(task->statProcess, evt->statProcess, mess);
}

void statsEVT_DESTROY(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                      ocrGuid_t evtGuid, ocrEvent_t *evt) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!evt) {
        deguidify(pd, evtGuid, (u64*)&evt, NULL);
    }
    
    if(edtGuid == NULL_GUID) {
        stats->fctPtrs->destructStatsProcess(stats, evt->statProcess);
        return;
    }
    
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_EVT_DESTROY, edtGuid,
                                                            evtGuid, NULL);
    ocrStatsAsyncMessage(task->statProcess, evt->statProcess, mess);
    stats->fctPtrs->destructStatsProcess(stats, evt->statProcess);
}

void statsDEP_SATISFYToEvt(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task,
                           ocrGuid_t evtGuid, ocrEvent_t *evt, ocrGuid_t dbGuid, u32 slot) {

    ocrStats_t *stats = pd->getStats(pd);

    if(edtGuid == NULL_GUID)
        return;

    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    if(!evt) {
        deguidify(pd, evtGuid, (u64*)&evt, NULL);
    }

    statsParamEvt_t params = { .dbPayload = dbGuid, .slot = slot };
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DEP_SATISFY, edtGuid, evtGuid, (ocrStatsParam_t*)&params);
    ocrStatsAsyncMessage(task->statProcess, evt->statProcess, mess);
}

void statsDEP_SATISFYFromEvt(ocrPolicyDomain_t *pd, ocrGuid_t evtGuid, ocrEvent_t *evt,
                             ocrGuid_t destGuid, ocrGuid_t dbGuid, u32 slot) {

    ocrStats_t *stats = pd->getStats(pd);
    ocrStatsProcess_t *dstProcess = NULL;

    if(!evt) {
        deguidify(pd, evtGuid, (u64*)&evt, NULL);
    }

    if(isEventGuid(destGuid)) {
        ocrEvent_t *tmp;
        deguidify(pd, destGuid, (u64*)&tmp, NULL);
        dstProcess = tmp->statProcess;
    } else if(isEdtGuid(destGuid)) {
        ocrTask_t *tmp;
        deguidify(pd, destGuid, (u64*)&tmp, NULL);
        dstProcess = tmp->statProcess;
    } else {
        ASSERT(0);
    }
        
    statsParamEvt_t params = { .dbPayload = dbGuid, .slot = slot };
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DEP_SATISFY, evtGuid, destGuid, (ocrStatsParam_t*)&params);
    ocrStatsAsyncMessage(evt->statProcess, dstProcess, mess);
}

// Slightly different API as well. We do not specify the depSrc (can be ocrEvent_t or ocrDatablock_t)
void statsDEP_ADD(ocrPolicyDomain_t *pd, ocrGuid_t edtGuid, ocrTask_t *task, ocrGuid_t depSrcGuid, ocrGuid_t depDestGuid,
                  ocrTask_t *depDest, u32 slot) {

    ocrStats_t *stats = pd->getStats(pd);
    ocrStatsProcess_t *midProcess = NULL;
    
    if(edtGuid == NULL_GUID)
        return;
    if(!task) {
        deguidify(pd, edtGuid, (u64*)&task, NULL);
    }
    if(!depDest) {
        deguidify(pd, depDestGuid, (u64*)&depDest, NULL);
    }

    if(isDatablockGuid(depSrcGuid)) {
        ocrDataBlock_t *tmp = NULL;
        deguidify(pd, depSrcGuid, (u64*)&tmp, NULL);
        midProcess = tmp->statProcess;
    } else if(isEventGuid(depSrcGuid)) {
        ocrEvent_t *tmp = NULL;
        deguidify(pd, depSrcGuid, (u64*)&tmp, NULL);
        midProcess = tmp->statProcess;
    } else {
        ASSERT(0);
    }
    statsParamEvt_t params = { .dbPayload = NULL_GUID, .slot = slot };
    
    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_DEP_ADD, edtGuid, depSrcGuid, (ocrStatsParam_t*)&params);
    ocrStatsAsyncMessage(task->statProcess, midProcess, mess);
    mess = stats->fctPtrs->createMessage(stats, STATS_DEP_ADD, depSrcGuid, depDestGuid, (ocrStatsParam_t*)&params);
    ocrStatsAsyncMessage(midProcess, depDest->statProcess, mess);
}

void statsWORKER_START(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                       ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!worker) {
        deguidify(pd, workerGuid, (u64*)&worker, NULL);
    }
    if(!compTarget) {
        deguidify(pd, compTargetGuid, (u64*)&compTarget, NULL);
    }

    worker->statProcess = stats->fctPtrs->createStatsProcess(stats, workerGuid);
    
    ASSERT(workerGuid == worker->guid);
    ASSERT(compTargetGuid == compTarget->guid);

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_WORKER_START, workerGuid, compTargetGuid, NULL);
    ocrStatsSyncMessage(worker->statProcess, compTarget->statProcess, mess);
}

void statsWORKER_STOP(ocrPolicyDomain_t *pd, ocrGuid_t workerGuid, ocrWorker_t *worker,
                      ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget) {
    
    ocrStats_t *stats = pd->getStats(pd);
    if(!worker) {
        deguidify(pd, workerGuid, (u64*)&worker, NULL);
    }
    if(!compTarget) {
        deguidify(pd, compTargetGuid, (u64*)&compTarget, NULL);
    }

    ASSERT(workerGuid == worker->guid);
    ASSERT(compTargetGuid == compTarget->guid);

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_WORKER_STOP, workerGuid, compTargetGuid, NULL);
    ocrStatsSyncMessage(worker->statProcess, compTarget->statProcess, mess);
    stats->fctPtrs->destructStatsProcess(stats, worker->statProcess);

}

void statsALLOCATOR_START(ocrPolicyDomain_t *pd, ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                          ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!allocator) {
        deguidify(pd, allocatorGuid, (u64*)&allocator, NULL);
    }
    if(!memTarget) {
        deguidify(pd, memTargetGuid, (u64*)&memTarget, NULL);
    }
    
    allocator->statProcess = stats->fctPtrs->createStatsProcess(stats, allocatorGuid);

    ASSERT(allocatorGuid == allocator->guid);
    ASSERT(memTargetGuid == memTarget->guid);

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_ALLOCATOR_START, allocatorGuid, memTargetGuid, NULL);
    ocrStatsSyncMessage(allocator->statProcess, memTarget->statProcess, mess);
}

void statsALLOCATOR_STOP(ocrPolicyDomain_t *pd, ocrGuid_t allocatorGuid, ocrAllocator_t *allocator,
                         ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget) {

    ocrStats_t *stats = pd->getStats(pd);
    if(!allocator) {
        deguidify(pd, allocatorGuid, (u64*)&allocator, NULL);
    }
    if(!memTarget) {
        deguidify(pd, memTargetGuid, (u64*)&memTarget, NULL);
    }
    
    ASSERT(allocatorGuid == allocator->guid);
    ASSERT(memTargetGuid == memTarget->guid);

    ocrStatsMessage_t *mess = stats->fctPtrs->createMessage(stats, STATS_ALLOCATOR_STOP, allocatorGuid, memTargetGuid, NULL);
    ocrStatsSyncMessage(allocator->statProcess, memTarget->statProcess, mess);
    
    stats->fctPtrs->destructStatsProcess(stats, allocator->statProcess);

}

void statsCOMPTARGET_START(ocrPolicyDomain_t *pd, ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget) {

    ocrStats_t *stats = pd->getStats(pd);

    if(!compTarget) {
        deguidify(pd, compTargetGuid, (u64*)&compTarget, NULL);
    }

    ASSERT(compTargetGuid == compTarget->guid);

    compTarget->statProcess = stats->fctPtrs->createStatsProcess(stats, compTargetGuid);
}

void statsCOMPTARGET_STOP(ocrPolicyDomain_t *pd, ocrGuid_t compTargetGuid, ocrCompTarget_t *compTarget) {
    
    ocrStats_t *stats = pd->getStats(pd);

    if(!compTarget) {
        deguidify(pd, compTargetGuid, (u64*)&compTarget, NULL);
    }

    ASSERT(compTargetGuid == compTarget->guid);

    stats->fctPtrs->destructStatsProcess(stats, compTarget->statProcess);
}

void statsMEMTARGET_START(ocrPolicyDomain_t *pd, ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget) {
    
    ocrStats_t *stats = pd->getStats(pd);

    if(!memTarget) {
        deguidify(pd, memTargetGuid, (u64*)&memTarget, NULL);
    }

    ASSERT(memTargetGuid == memTarget->guid);

    memTarget->statProcess = stats->fctPtrs->createStatsProcess(stats, memTargetGuid);
}

void statsMEMTARGET_STOP(ocrPolicyDomain_t *pd, ocrGuid_t memTargetGuid, ocrMemTarget_t *memTarget) {

    ocrStats_t *stats = pd->getStats(pd);

    if(!memTarget) {
        deguidify(pd, memTargetGuid, (u64*)&memTarget, NULL);
    }

    ASSERT(memTargetGuid == memTarget->guid);

    stats->fctPtrs->destructStatsProcess(stats, memTarget->statProcess);
}

#endif /* OCR_ENABLE_STATISTICS */
