#include "ocr-runtime.h"
/*
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
*/
#include "machine-description/ocr-machine.h"

const char *type_str[] = {
    "GuidType",
    "MemPlatformType",
    "MemTargetType",
    "AllocatorType",
    "CompPlatformType",
    "CompTargetType",
    "WorkPileType",
    "WorkerType",
    "SchedulerType",
    "PolicyDomainType",
};

const char *inst_str[] = {
    "GuidInst",
    "MemPlatformInst",
    "MemTargetInst",
    "AllocatorInst",
    "CompPlatformInst",
    "CompTargetInst",
    "WorkPileInst",
    "WorkerInst",
    "SchedulerInst",
    "PolicyDomainInst",
};

/* The above struct defines dependence "from" -> "to" using "refstr" as reference */
/* The below array defines the list of dependences */

dep_t deps[] = {
    { memtarget_type, memplatform_type, "memplatform"},
    { allocator_type, memtarget_type, "memtarget"},
    { comptarget_type, compplatform_type, "compplatform"},
    { worker_type, comptarget_type, "comptarget"},
    { scheduler_type, workpile_type, "workpile"},
    { scheduler_type, worker_type, "worker"},
    { policydomain_type, guid_type, "guid"},
    { policydomain_type, memtarget_type, "memtarget"},
    { policydomain_type, allocator_type, "allocator"},
    { policydomain_type, comptarget_type, "comptarget"},
    { policydomain_type, workpile_type, "workpile"},
    { policydomain_type, worker_type, "worker"},
    { policydomain_type, scheduler_type, "scheduler"},
};

extern char* populate_type(ocrParamList_t *type_param, type_enum index, dictionary *dict, char *secname);
int populate_inst(ocrParamList_t **inst_param, ocrMappable_t **instance, int *type_counts, char ***factory_names, void ***all_factories, ocrMappable_t ***all_instances, type_enum index, int inst_index, int inst_count, dictionary *dict, char *secname);
extern int build_deps (dictionary *dict, int A, int B, char *refstr, ocrMappable_t ***all_instances, ocrParamList_t ***inst_params);
extern void *create_factory (type_enum index, char *factory_name, ocrParamList_t *paramlist);
extern int read_range(dictionary *dict, char *sec, char *field, int *low, int *high);
extern ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]);
extern void ocrStop();

void hack() {
    int i, j, count;
    //dictionary *dict = iniparser_load(argv[1]);
    dictionary *dict = iniparser_load("static.cfg");

    int type_counts[sizeof(type_str)/sizeof(const char *)];
    ocrParamList_t **type_params[sizeof(type_str)/sizeof(const char *)];
    char **factory_names[sizeof(type_str)/sizeof(const char *)];        // ~9 different kinds (memtarget, comptarget, etc.); each with diff names (tlsf, malloc, etc.); each pointing to a char*
    void **all_factories[sizeof(type_str)/sizeof(const char *)];

    int inst_counts[sizeof(inst_str)/sizeof(const char *)];
    ocrParamList_t **inst_params[sizeof(inst_str)/sizeof(const char *)];
    ocrMappable_t **all_instances[sizeof(inst_str)/sizeof(const char *)];
    int total_types = sizeof(type_str)/sizeof(const char *);

    // INIT
        for (j = 0; j < total_types; j++) {
        type_params[j] = NULL; type_counts[j] = 0; factory_names[j] = NULL;
        inst_params[j] = NULL; inst_counts[j] = 0;
        all_factories[j] = NULL; all_instances[j] = NULL;
    }

    // POPULATE TYPES
   printf("========= Create factories ==========\n");
    for (i = 0; i < iniparser_getnsec(dict); i++)
        for (j = 0; j < total_types; j++)
            if (strncasecmp(type_str[j], iniparser_getsecname(dict, i), strlen(type_str[j]))==0) type_counts[j]++;

    for (i = 0; i < iniparser_getnsec(dict); i++) {
        for (j = 0; j < total_types; j++) {
            if (strncasecmp(type_str[j], iniparser_getsecname(dict, i), strlen(type_str[j]))==0) {
                if(type_counts[j] && type_params[j]==NULL) {
                    type_params[j] = (ocrParamList_t **)malloc(type_counts[j] * sizeof(ocrParamList_t *));
                    factory_names[j] = (char **)malloc(type_counts[j] * sizeof(char *));
                    all_factories[j] = (void **)malloc(type_counts[j] * sizeof(void *));
                    count = 0;
                }
                factory_names[j][count] = populate_type(type_params[j][count], j, dict, iniparser_getsecname(dict, i));
                all_factories[j][count] = create_factory(j, factory_names[j][count], type_params[j][count]);
                if (all_factories[j][count] == NULL) {
                    free(factory_names[j][count]);
                    factory_names[j][count] = NULL;
                }
                count++;
            }
        }
    }

    printf("\n\n");

    // POPULATE INSTANCES
    printf("========= Create instances ==========\n");
    for (i = 0; i < iniparser_getnsec(dict); i++)
        for (j = 0; j < total_types; j++)
            if (strncasecmp(inst_str[j], iniparser_getsecname(dict, i), strlen(inst_str[j]))==0) {
                int low, high, count;
                count = read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
                inst_counts[j]+=count;
            }

    for (i = 0; i < iniparser_getnsec(dict); i++) {
        for (j = total_types-1; j >= 0; j--) {
            if (strncasecmp(inst_str[j], iniparser_getsecname(dict, i), strlen(inst_str[j]))==0) {
                if(inst_counts[j] && inst_params[j] == NULL) {
                    printf("Create %d instances of %s\n", inst_counts[j], inst_str[j]);
                    inst_params[j] = (ocrParamList_t **)malloc(inst_counts[j] * sizeof(ocrParamList_t *));
                    all_instances[j] = (ocrMappable_t **)malloc(inst_counts[j] * sizeof(ocrMappable_t *));
                    count = 0;
                }
                populate_inst(inst_params[j], all_instances[j], type_counts, factory_names, all_factories, all_instances, j, count++, inst_counts[j], dict, iniparser_getsecname(dict, i));
            }
        }
    }

    // FIXME: Ugly follows
   ocrCompPlatformFactory_t *compPlatformFactory;
    compPlatformFactory = (ocrCompPlatformFactory_t *) all_factories[4][0];
    compPlatformFactory->setIdentifyingFunctions(compPlatformFactory);

    printf("\n\n");

    // BUILD DEPENDENCES
    printf("========= Build dependences ==========\n");
    for (i = 0; i < sizeof(deps)/sizeof(dep_t); i++) {
        build_deps(dict, deps[i].from, deps[i].to, deps[i].refstr, all_instances, inst_params);
    }
    
    printf("\n\n");

    // START EXECUTION
    printf("========= Start execution ==========\n");

    ocrPolicyDomain_t *rootPolicy;
    rootPolicy = (ocrPolicyDomain_t *) all_instances[9][0]; // FIXME: Ugly
    rootPolicy->start(rootPolicy);
    // We now create the EDT and launch it
    ocrGuid_t edtTemplateGuid, edtGuid;
    ocrEdtTemplateCreate(&edtTemplateGuid, mainEdt, 0, 0);
    ocrEdtCreate(&edtGuid, edtTemplateGuid, 0, /* paramv = */ NULL,
                /* depc = */ 0, /* depv = */ NULL,
                EDT_PROP_NONE, NULL_GUID, NULL);
    ocrStop();
    
}

#if 0
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

    allocator->memoryCount = 1;
    allocator->memories = (ocrMemTarget_t**)malloc(sizeof(ocrMemTarget_t*));
    allocator->memories[0] = memTarget;

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
#endif


