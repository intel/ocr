#include "ocr-runtime.h"

#include "policy-domain/policy-domain-all.h"
#include "hc.h"
#include "task/task-all.h"
#include "datablock/datablock-all.h"
#include "event/event-all.h"
#include "guid/guid-all.h"
#include "sync/sync-all.h"
#include "mem-platform/mem-platform-all.h"
#include "comp-platform/comp-platform-all.h"
#include "mem-target/mem-target-all.h"
#include "comp-target/comp-target-all.h"
#include "allocator/allocator-all.h"
#include "scheduler/scheduler-all.h"
#include "worker/worker-all.h"
#include "workpile/workpile-all.h"


extern ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);
extern void ocrStop();


void hack() {


    ocrParamList_t* policyDomainFactoryParams = NULL;
    ocrPolicyDomainFactory_t * policyDomainFactory = newPolicyDomainFactory ( policyDomainHc_id, policyDomainFactoryParams );

    ocrParamList_t* taskFactoryParams = NULL;
    ocrTaskFactory_t *taskFactory = newTaskFactoryHc ( taskFactoryParams );

    ocrParamList_t* taskTemplateFactoryParams = NULL;
    ocrTaskTemplateFactory_t *taskTemplateFactory = newTaskTemplateFactoryHc ( taskTemplateFactoryParams );

    ocrParamList_t* dataBlockFactoryParams = NULL;
    ocrDataBlockFactory_t *dbFactory = newDataBlockFactoryRegular ( dataBlockFactoryParams );

    void* eventFactoryConfigParams = NULL;
    ocrEventFactory_t *eventFactory = newEventFactoryHc ( eventFactoryConfigParams );

    ocrParamList_t* policyContextFactoryParams = NULL;
    ocrPolicyCtxFactory_t *contextFactory = newPolicyContextFactoryHC ( policyContextFactoryParams );

    ocrParamList_t* guidProviderFactoryParams = NULL;
    ocrGuidProviderFactory_t *guidFactory = newGuidProviderFactoryPtr(guidProviderFactoryParams);
    ocrGuidProvider_t *guidProvider = guidFactory->instantiate(guidFactory, NULL);

    ocrParamList_t* lockFactoryParams = NULL;
    ocrLockFactory_t *lockFactory = newLockFactoryX86(lockFactoryParams);

    ocrParamList_t* atomicFactoryParams = NULL;
    ocrAtomic64Factory_t *atomicFactory = newAtomic64FactoryX86(atomicFactoryParams);

    ocrCost_t *costFunction = NULL;

    ocrPolicyDomain_t * rootPolicy = policyDomainFactory->instantiate(
        policyDomainFactory /*factory*/,
        1 /*schedulerCount*/, 1/*workerCount*/, 1 /*computeCount*/,
        1 /*workpileCount*/, 1 /*allocatorCount*/, 1 /*memoryCount*/,
        taskFactory, taskTemplateFactory, dbFactory,
        eventFactory, contextFactory, guidProvider, lockFactory,
        atomicFactory, costFunction, NULL
        );

    setBootPD(rootPolicy);

    // Get the platforms
    ocrParamList_t *memPlatformFactoryParams = NULL;
    ocrMemPlatformFactory_t *memPlatformFactory = newMemPlatformFactoryMalloc(memPlatformFactoryParams);

    // The first comp-platform represents the master thread,
    // others should have isMasterThread set to false
    ocrCompPlatformFactory_t *compPlatformFactory = newCompPlatformFactoryPthread(NULL);
    ocrMemPlatform_t *memPlatform = memPlatformFactory->instantiate(memPlatformFactory, NULL);
    // Get the master plaform instance
    paramListCompPlatformPthread_t compPlatformMasterParams;
    compPlatformMasterParams.isMasterThread = true;
    compPlatformMasterParams.stackSize = 0; // will get pthread's default
    ocrCompPlatform_t *compPlatform = compPlatformFactory->instantiate(compPlatformFactory, (ocrParamList_t *) &compPlatformMasterParams);

    // Now get the target factories and instances
    ocrParamList_t *memTargetFactoryParams = NULL;
    ocrMemTargetFactory_t *memTargetFactory = newMemTargetFactoryShared(memTargetFactoryParams);

    ocrParamList_t *compTargetFactoryParams = NULL;
    ocrCompTargetFactory_t *compTargetFactory = newCompTargetFactoryHc(compTargetFactoryParams);

    ocrMemTarget_t *memTarget = memTargetFactory->instantiate(memTargetFactory, NULL);
    ocrCompTarget_t *compTarget = compTargetFactory->instantiate(compTargetFactory, NULL);

    // Link the platform and target
    memTarget->memoryCount = 1;
    memTarget->memories = (ocrMemPlatform_t**)malloc(sizeof(ocrMemPlatform_t*));
    memTarget->memories[0] = memPlatform;

    compTarget->platformCount = 1;
    compTarget->platforms = (ocrCompPlatform_t**)malloc(sizeof(ocrCompPlatform_t*));
    compTarget->platforms[0] = compPlatform;

    // Create allocator and worker
    paramListAllocatorInst_t *allocatorParamList = (paramListAllocatorInst_t*)malloc(sizeof(paramListAllocatorInst_t));
    allocatorParamList->size = 32*1024*1024;
    ocrAllocatorFactory_t *allocatorFactory = newAllocatorFactoryTlsf(NULL);
    ocrAllocator_t *allocator = allocatorFactory->instantiate(allocatorFactory,
                                                              (ocrParamList_t*)allocatorParamList);
    ((ocrMappable_t*)allocator)->mapFct((ocrMappable_t*)allocator, OCR_MEM_TARGET, 1,
                                        (ocrMappable_t**)&memTarget);

    ocrWorkerFactory_t * workerFactory = newOcrWorkerFactoryHc(NULL);
    paramListWorkerHcInst_t workerParam;
    workerParam.workerId = 0;
    ocrWorker_t *worker = workerFactory->instantiate(workerFactory, (ocrParamList_t *) &workerParam);
    worker->computeCount = 1;
    worker->computes = (ocrCompTarget_t**)malloc(sizeof(ocrCompTarget_t*));
    worker->computes[0] = compTarget;

    // Create workpile and scheduler
    ocrWorkpileFactory_t *workpileFactory = newOcrWorkpileFactoryHc(NULL);
    ocrWorkpile_t* workpile = workpileFactory->instantiate(workpileFactory, NULL);

    paramListSchedulerHcInst_t *schedulerParamList = (paramListSchedulerHcInst_t*)malloc(sizeof(paramListSchedulerHcInst_t));
    schedulerParamList->worker_id_first = 0;
    ocrSchedulerFactory_t *schedulerFactory = newOcrSchedulerFactoryHc(NULL);
    ocrScheduler_t *scheduler = schedulerFactory->instantiate(schedulerFactory,
                                                              (ocrParamList_t*)schedulerParamList);

    scheduler->workpileCount = 1;
    scheduler->workpiles = (ocrWorkpile_t**)malloc(sizeof(ocrWorkpile_t*));
    scheduler->workpiles[0] = workpile;

    scheduler->workerCount = 1;
    scheduler->workers = (ocrWorker_t**)malloc(sizeof(ocrWorker_t*));
    scheduler->workers[0] = worker;

    // Now set things up with the policy domain
    rootPolicy->schedulers = (ocrScheduler_t**)malloc(sizeof(ocrScheduler_t*));
    rootPolicy->schedulers[0] = scheduler;

    rootPolicy->workers = (ocrWorker_t**)malloc(sizeof(ocrWorker_t*));
    rootPolicy->workers[0] = worker;

    rootPolicy->computes = (ocrCompTarget_t**)malloc(sizeof(ocrCompTarget_t*));
    rootPolicy->computes[0] = compTarget;

    rootPolicy->workpiles = (ocrWorkpile_t**)malloc(sizeof(ocrWorkpile_t*));
    rootPolicy->workpiles[0] = workpile;

    rootPolicy->allocators = (ocrAllocator_t**)malloc(sizeof(ocrAllocator_t*));
    rootPolicy->allocators[0] = allocator;

    rootPolicy->memories = (ocrMemTarget_t**)malloc(sizeof(ocrMemTarget_t*));
    rootPolicy->memories[0] = memTarget;

    compPlatformFactory->setIdentifyingFunctions(compPlatformFactory);

    rootPolicy->start(rootPolicy);
    // We now create the EDT and launch it
    ocrGuid_t edtTemplateGuid, edtGuid;
    ocrEdtTemplateCreate(&edtTemplateGuid, mainEdt, 0, 0);
    ocrEdtCreate(&edtGuid, edtTemplateGuid, 0, /* paramv = */ NULL,
                 /* depc = */ 0, /* depv = */ NULL,
                 EDT_PROP_NONE, NULL_GUID, NULL);
    ocrStop();
}
