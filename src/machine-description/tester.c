#include <stdio.h>
#include <string.h>
#include <iniparser.h>
#include <signal.h>

#include <ocr-mem-platform.h>
#include <ocr-mem-target.h>
#include <ocr-allocator.h>
#include <ocr-comp-platform.h>
#include <ocr-comp-target.h>
#include <ocr-worker.h>
#include <ocr-workpile.h>
#include <ocr-scheduler.h>
#include <ocr-policy-domain.h>
#include <ocr-datablock.h>
#include <ocr-event.h>

#include <mem-platform/mem-platform-all.h>
#include <comp-platform/comp-platform-all.h>
#include <mem-target/mem-target-all.h>
#include <allocator/allocator-all.h>
#include <comp-target/comp-target-all.h>
#include <workpile/workpile-all.h>
#include <worker/worker-all.h>
#include <scheduler/scheduler-all.h>
#include <policy-domain/policy-domain-all.h>
#include <task/task-all.h>
#include <event/event-all.h>
#include <guid/guid-all.h>
#include <datablock/datablock-all.h>
#include <sync/sync-all.h>

#define INI_GET_INT(KEY, VAR, DEF) VAR = (int) iniparser_getint(dict, KEY, DEF); if (VAR==DEF){ printf("Key %s not found or invalid!\n", KEY); }
#define INI_GET_STR(KEY, VAR, DEF) VAR = (char *) iniparser_getstring(dict, KEY, DEF); if (!strcmp(VAR, DEF)){ printf("Key %s not found or invalid!\n", KEY); }
#define TO_ENUM(VAR, STR, TYPE, TYPESTR, COUNT) {int kount; for (kount=0; kount<COUNT; kount++) {if (!strcmp(STR, TYPESTR[kount])) VAR=(TYPE)kount;} }
#define INI_IS_RANGE(KEY) strstr((char *) iniparser_getstring(dict, KEY, ""), "-")
#define INI_GET_RANGE(KEY, LOW, HIGH) sscanf(iniparser_getstring(dict, KEY, ""), "%d-%d", &LOW, &HIGH)

ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[])
{
 ocrShutdown();
 return NULL_GUID;
}

//TODO: expand all the below to include all sections
typedef enum {
    guid_type,
    memplatform_type,
    memtarget_type,
    allocator_type,
    complatform_type,
    comptarget_type,
    workpile_type,
    worker_type,
    scheduler_type,
    policydomain_type,
} type_enum;

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

int type_counts[sizeof(type_str)/sizeof(const char *)];
ocrParamList_t **type_params[sizeof(type_str)/sizeof(const char *)];
char **factory_names[sizeof(type_str)/sizeof(const char *)];	// ~9 different kinds (memtarget, comptarget, etc.); each with diff names (tlsf, malloc, etc.); each pointing to a char*
void **all_factories[sizeof(type_str)/sizeof(const char *)];

typedef enum {
    guid_inst,
    memplatform_inst,
    memtarget_inst,
    allocator_inst,
    compplatform_inst,
    comptarget_inst,
    workpile_inst,
    worker_inst,
    scheduler_inst,
    policydomain_inst,
} inst_enum;

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

int inst_counts[sizeof(inst_str)/sizeof(const char *)];
ocrParamList_t **inst_params[sizeof(inst_str)/sizeof(const char *)];
ocrMappable_t **all_instances[sizeof(inst_str)/sizeof(const char *)];

typedef struct {
    int from;
    int to;
    char *refstr;
} dep_t;

/* The above struct defines dependency "from" -> "to" using "refstr" as reference */

dep_t deps[] = {
    { 2, 1, "memplatform"},
    { 3, 2, "memtarget"},
    { 5, 4, "compplatform"},
    { 7, 5, "comptarget"},
    { 8, 6, "workpile"},
    { 8, 7, "worker"},
    { 9, 0, "guid"},
    { 9, 2, "memtarget"},
    { 9, 3, "allocator"},
    { 9, 5, "comptarget"},
    { 9, 6, "workpile"},
    { 9, 7, "worker"},
    { 9, 8, "scheduler"},
};

// TODO: expand to parse comma separated values & ranges iterating the below thru strtok with ,
// TODO: stretch goal, extend this to expressions: surely you're joking, Mr. Feynman
int read_range(dictionary *dict, char *sec, char *field, int *low, int *high)
{
    char key[64];
    int value;
    int lo, hi;
    int count;

    snprintf(key, 64, "%s:%s", sec, field);
    if (INI_IS_RANGE(key)) {
        INI_GET_RANGE(key, lo, hi);
        count = hi-lo+1;
    } else {
        INI_GET_INT(key, value, -1);
        count = 1;
        lo = hi = value;
    }
    *low = lo; *high = hi;
    return count;
}

char* populate_type(ocrParamList_t **type_param, int index, int type_index, int type_count, dictionary *dict, char *secname)
{
    int i;
    char *typestr;
    char key[64];

    // TODO: populate type-specific fields
    switch (index) {
    case 0:
        ALLOC_PARAM_LIST(type_param[type_index], paramListGuidProviderFact_t);
        break;
    case 1:
        ALLOC_PARAM_LIST(type_param[type_index], paramListMemPlatformFact_t);
        break;
    case 2:
        ALLOC_PARAM_LIST(type_param[type_index], paramListMemTargetFact_t);
        break;
    case 3:
        ALLOC_PARAM_LIST(type_param[type_index], paramListAllocatorFact_t);
        break;
    case 4:
        ALLOC_PARAM_LIST(type_param[type_index], paramListCompPlatformFact_t);
        break;
    case 5:
        ALLOC_PARAM_LIST(type_param[type_index], paramListCompTargetFact_t);
        break;
    case 6:
        ALLOC_PARAM_LIST(type_param[type_index], paramListWorkpileFact_t);
        break;
    case 7:
        ALLOC_PARAM_LIST(type_param[type_index], paramListWorkerFact_t);
        break;
    case 8:
        ALLOC_PARAM_LIST(type_param[type_index], paramListSchedulerFact_t);
        break;
    case 9:
        ALLOC_PARAM_LIST(type_param[type_index], paramListPolicyDomainFact_t);
        break;
    default:
        printf("Error: %d index unexpected\n", type_index);
        break;
    }

     snprintf(key, 64, "%s:%s", secname, "name");
     INI_GET_STR (key, typestr, "");
    // TODO: change this to secname:misc type_param[type_index]->misc = strdup(typestr);
    return strdup(typestr);
}

ocrCompPlatformFactory_t *create_factory_compplatform(char *name, ocrParamList_t *paramlist)
{
    compPlatformType_t mytype = -1;
    TO_ENUM (mytype, name, compPlatformType_t, compplatform_types, compPlatformMax_id);

    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a compplatform factory of type %d: %s\n", mytype, factory_names[4][mytype]);
        return newCompPlatformFactory(mytype, paramlist);
    }
}

ocrMemPlatformFactory_t *create_factory_memplatform(char *name, ocrParamList_t *paramlist)
{
    memPlatformType_t mytype = -1;
    TO_ENUM (mytype, name, memPlatformType_t, memplatform_types, memPlatformMax_id);

    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a memplatform factory of type %d: %s\n", mytype, factory_names[1][mytype]);
        return newMemPlatformFactory(mytype, paramlist);
    }
}

ocrMemPlatformFactory_t *create_factory_memtarget(char *name, ocrParamList_t *paramlist)
{
    memTargetType_t mytype = -1;
    TO_ENUM (mytype, name, memTargetType_t, memtarget_types, memTargetMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a memtarget factory of type %d: %s\n", mytype, factory_names[2][mytype]);
        return (ocrMemPlatformFactory_t *)newMemTargetFactory(mytype, paramlist);
    }
}

ocrAllocatorFactory_t *create_factory_allocator(char *name, ocrParamList_t *paramlist)
{
    allocatorType_t mytype = -1;
    TO_ENUM (mytype, name, allocatorType_t, allocator_types, allocatorMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating an allocator factory of type %d: %s\n", mytype, factory_names[3][mytype]);
        return (ocrAllocatorFactory_t *)newAllocatorFactory(mytype, paramlist);
    }
}

ocrCompTargetFactory_t *create_factory_comptarget(char *name, ocrParamList_t *paramlist)
{
    compTargetType_t mytype = -1;
    TO_ENUM (mytype, name, compTargetType_t, comptarget_types, compTargetMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a comptarget factory of type %d: %s\n", mytype, factory_names[5][mytype]);
        return (ocrCompTargetFactory_t *)newCompTargetFactory(mytype, paramlist);
    }
}

ocrWorkpileFactory_t *create_factory_workpile(char *name, ocrParamList_t *paramlist)
{
    workpileType_t mytype = -1;
    TO_ENUM (mytype, name, workpileType_t, workpile_types, workpileMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a workpile factory of type %d: %s\n", mytype, factory_names[6][mytype]);
        return (ocrWorkpileFactory_t *)newWorkpileFactory(mytype, paramlist);
    }
}

ocrWorkerFactory_t *create_factory_worker(char *name, ocrParamList_t *paramlist)
{
    workerType_t mytype = -1;
    TO_ENUM (mytype, name, workerType_t, worker_types, workerMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a worker factory of type %d: %s\n", mytype, factory_names[7][mytype]);
        return (ocrWorkerFactory_t *)newWorkerFactory(mytype, paramlist);
    }
}

ocrSchedulerFactory_t *create_factory_scheduler(char *name, ocrParamList_t *paramlist)
{
    schedulerType_t mytype = -1;
    TO_ENUM (mytype, name, schedulerType_t, scheduler_types, schedulerMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a scheduler factory of type %d: %s\n", mytype, factory_names[8][mytype]);
        return (ocrSchedulerFactory_t *)newSchedulerFactory(mytype, paramlist);
    }
}

ocrPolicyDomainFactory_t *create_factory_policydomain(char *name, ocrParamList_t *paramlist)
{
    policyDomainType_t mytype = -1;
    TO_ENUM (mytype, name, policyDomainType_t, policyDomain_types, policyDomainMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a worker factory of type %d: %s\n", mytype, factory_names[9][mytype]);
        return (ocrPolicyDomainFactory_t *)newPolicyDomainFactory(mytype, paramlist);
    }
}

ocrTaskFactory_t *create_factory_task(char *name, ocrParamList_t *paramlist)
{
    taskType_t mytype = -1;
    TO_ENUM (mytype, name, taskType_t, task_types, taskMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a task factory of type %d\n", mytype);
        return (ocrTaskFactory_t *)newTaskFactory(mytype, paramlist);
    }
}

ocrTaskTemplateFactory_t *create_factory_tasktemplate(char *name, ocrParamList_t *paramlist)
{
    taskTemplateType_t mytype = -1;
    TO_ENUM (mytype, name, taskTemplateType_t, taskTemplate_types, taskTemplateMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a task template factory of type %d\n", mytype);
        return (ocrTaskTemplateFactory_t *)newTaskTemplateFactory(mytype, paramlist);
    }
}

ocrDataBlockFactory_t *create_factory_datablock(char *name, ocrParamList_t *paramlist)
{
    dataBlockType_t mytype = -1;
    TO_ENUM (mytype, name, dataBlockType_t, dataBlock_types, dataBlockMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a datablock factory of type %d\n", mytype);
        return (ocrDataBlockFactory_t *)newDataBlockFactory(mytype, paramlist);
    }
}

ocrEventFactory_t *create_factory_event(char *name, ocrParamList_t *paramlist)
{
    eventType_t mytype = -1;
    TO_ENUM (mytype, name, eventType_t, event_types, eventMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating an event factory of type %d\n", mytype);
        return (ocrEventFactory_t *)newEventFactory(mytype, paramlist);
    }
}

ocrPolicyCtxFactory_t *create_factory_context(char *name, ocrParamList_t *paramlist)
{
    policyCtxType_t mytype = -1;
    TO_ENUM (mytype, name, policyCtxType_t, policyCtx_types, policyCtxMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a policy ctx factory of type %d\n", mytype);
        return (ocrPolicyCtxFactory_t *)newPolicyCtxFactory(mytype, paramlist);
    }
}

ocrGuidProviderFactory_t *create_factory_guid(char *name, ocrParamList_t *paramlist)
{
    guidType_t mytype = -1;
    TO_ENUM (mytype, name, guidType_t, guid_types, guidMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a guid factory of type %d\n", mytype);
        return (ocrGuidProviderFactory_t *)newGuidProviderFactory(mytype, paramlist);
    }
}

ocrLockFactory_t *create_factory_lock(char *name, ocrParamList_t *paramlist)
{
    syncType_t mytype = -1;
    TO_ENUM (mytype, name, syncType_t, sync_types, syncMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating a lock factory of type %d\n", mytype);
        return (ocrLockFactory_t *)newLockFactory(mytype, paramlist);
    }
}

ocrAtomic64Factory_t *create_factory_atomic64(char *name, ocrParamList_t *paramlist)
{
    syncType_t mytype = -1;
    TO_ENUM (mytype, name, syncType_t, sync_types, syncMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else {
        printf("Creating an atomic64 factory of type %d\n", mytype);
        return (ocrAtomic64Factory_t *)newAtomic64Factory(mytype, paramlist);
    }
}

void *create_factory (int index, char *factory_name, ocrParamList_t *paramlist)
{
    void *new_factory;

    switch (index) {
    case 0:
        new_factory = (void *)create_factory_guid(factory_name, paramlist);
        break;
    case 1:
        new_factory = (void *)create_factory_memplatform(factory_name, paramlist);
        break;
    case 2:
        new_factory = (void *)create_factory_memtarget(factory_name, paramlist);
        break;
    case 3:
        new_factory = (void *)create_factory_allocator(factory_name, paramlist);
        break;
    case 4:
        new_factory = (void *)create_factory_compplatform(factory_name, paramlist);
        break;
    case 5:
        new_factory = (void *)create_factory_comptarget(factory_name, paramlist);
        break;
    case 6:
        new_factory = (void *)create_factory_workpile(factory_name, paramlist);
        break;
    case 7:
        new_factory = (void *)create_factory_worker(factory_name, paramlist);
        break;
    case 8:
        new_factory = (void *)create_factory_scheduler(factory_name, paramlist);
        break;
    case 9:
        new_factory = (void *)create_factory_policydomain(factory_name, paramlist);
        break;
    default:
        printf("Error: %d index unexpected\n", index);
        break;
    }
}

int populate_inst(ocrParamList_t **inst_param, ocrMappable_t **instance, int index, int inst_index, int inst_count, dictionary *dict, char *secname)
{
    int i, low, high, count, j;
    char *inststr;
    char key[64];
    void *factory;

    count = read_range(dict, secname, "id", &low, &high);

    snprintf(key, 64, "%s:%s", secname, "type");
    INI_GET_STR (key, inststr, "");

    for (i = 0; i < type_counts[index]; i++) {
        if (factory_names[index][i] && (0 == strncmp(factory_names[index][i], inststr, strlen(factory_names[index][i])))) break;
    }
    if (factory_names[index][i] == NULL || strncmp(factory_names[index][i], inststr, strlen(factory_names[index][i]))) {
        printf("Unknown type %s\n", inststr);
        return 0;
    }

    // find factory based on i
    factory = all_factories[index][i];

    // Use the factory's instantiate() to create instance

    switch (index) {
    case 0:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListGuidProviderInst_t);
            instance[j] = (ocrMappable_t *)((ocrGuidProviderFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created guid provider of type %s, index %d\n", inststr, j);
        }
        break;
    case 1:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemPlatformInst_t);
            instance[j] = (ocrMappable_t *)((ocrMemPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created memplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case 2:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemTargetInst_t);
            instance[j] = (ocrMappable_t *)((ocrMemTargetFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created memtarget of type %s, index %d\n", inststr, j);
        }
        break;
    case 3:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListAllocatorInst_t);
            ((paramListAllocatorInst_t *)inst_param[j])->size = 33554432;
            instance[j] = (ocrMappable_t *)((ocrAllocatorFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created allocator of type %s, index %d\n", inststr, j);
        }
        break;
    case 4:
        for (j = low; j<=high; j++) {
            // ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformInst_t);
            // FIXME: for now assume it's HC
            ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformPthread_t);
            if (j == 0) {
                ((paramListCompPlatformPthread_t *)inst_param[j])->isMasterThread = true;
                ((paramListCompPlatformPthread_t *)inst_param[j])->stackSize = 0;
            }
            instance[j] = (ocrMappable_t *)((ocrCompPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created compplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case 5:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListCompTargetInst_t);
            instance[j] = (ocrMappable_t *)((ocrCompTargetFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created comptarget of type %s, index %d\n", inststr, j);
        }
        break;
    case 6:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListWorkpileInst_t);
            instance[j] = (ocrMappable_t *)((ocrWorkpileFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created workpile of type %s, index %d\n", inststr, j);
        }
        break;
    case 7:
        for (j = low; j<=high; j++) {
            //ALLOC_PARAM_LIST(inst_param[j], paramListWorkerInst_t);
            // FIXME: for now assume it's HC
            ALLOC_PARAM_LIST(inst_param[j], paramListWorkerHcInst_t);
            if (j == 0) {
                ((paramListWorkerHcInst_t *)inst_param[j])->workerId = 0;
            }
            instance[j] = (ocrMappable_t *)((ocrWorkerFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created worker of type %s, index %d\n", inststr, j);
        }
        break;
    case 8:
        for (j = low; j<=high; j++) {
            //ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerInst_t);
            // FIXME: for now assume it's HC
            ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerHcInst_t);
            if (j == 0) {
                ((paramListSchedulerHcInst_t *)inst_param[j])->workerIdFirst = 0;
            }
            instance[j] = (ocrMappable_t *)((ocrSchedulerFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j]) printf("Created scheduler of type %s, index %d\n", inststr, j);
        }
        break;
    case 9:
        for (j = low; j<=high; j++) {
            ocrTaskFactory_t *tf;
            ocrTaskTemplateFactory_t *ttf;
            ocrDataBlockFactory_t *dbf;
            ocrEventFactory_t *ef;
            ocrPolicyCtxFactory_t *cf;
            ocrGuidProvider_t *gf;
            ocrLockFactory_t *lf;
            ocrAtomic64Factory_t *af;
            int low, high;

            int schedulerCount, workerCount, computeCount, workpileCount, allocatorCount, memoryCount;
            schedulerCount = read_range(dict, secname, "scheduler", &low, &high);
            workerCount = read_range(dict, secname, "worker", &low, &high);
            computeCount = read_range(dict, secname, "comptarget", &low, &high);
            workpileCount = read_range(dict, secname, "workpile", &low, &high);
            allocatorCount = read_range(dict, secname, "allocator", &low, &high);
            memoryCount = read_range(dict, secname, "memtarget", &low, &high);

            snprintf(key, 64, "%s:%s", secname, "taskfactory");
            INI_GET_STR (key, inststr, "");
            tf = create_factory_task(inststr, NULL);

            snprintf(key, 64, "%s:%s", secname, "tasktemplatefactory");
            INI_GET_STR (key, inststr, "");
            ttf = create_factory_tasktemplate(inststr, NULL);

            snprintf(key, 64, "%s:%s", secname, "datablockfactory");
            INI_GET_STR (key, inststr, "");
            dbf = create_factory_datablock(inststr, NULL);

            snprintf(key, 64, "%s:%s", secname, "eventfactory");
            INI_GET_STR (key, inststr, "");
            ef = create_factory_event(inststr, NULL);

            snprintf(key, 64, "%s:%s", secname, "contextfactory");
            INI_GET_STR (key, inststr, "");
            cf = create_factory_context(inststr, NULL);

            snprintf(key, 64, "%s:%s", secname, "sync");
            INI_GET_STR (key, inststr, "");
            lf = create_factory_lock(inststr, NULL);

            snprintf(key, 64, "%s:%s", secname, "sync");
            INI_GET_STR (key, inststr, "");
            af = create_factory_atomic64(inststr, NULL);

            // Ugly but special case
            read_range(dict, secname, "guid", &low, &high);
            ASSERT (low == high);
            gf = (ocrGuidProvider_t *)(all_instances[0][low]); // I'm very unhappy with this - referencing global. FIXME
            ASSERT (gf);

            ALLOC_PARAM_LIST(inst_param[j], paramListPolicyDomainInst_t);
            instance[j] = (ocrMappable_t *)((ocrPolicyDomainFactory_t *)factory)->instantiate(factory, schedulerCount,
                            workerCount, computeCount, workpileCount, allocatorCount, memoryCount,
                            tf, ttf, dbf, ef, cf, gf, lf, af, NULL, inst_param[j]);
            if (instance[j]) printf("Created policy domain of index %d\n", j);
            setBootPD((ocrPolicyDomain_t *)instance[j]);
        }
        break;
    default:
        printf("Error: %d index unexpected\n", inst_index);
        break;
    }

    // Populate the params
    snprintf(key, 64, "%s:%s", secname, "type");
    INI_GET_STR (key, inststr, "");
    for (j = low; j <= high; j++) inst_param[j]->misc = strdup(inststr);

    // Sanity check that the type is already known
    for (j = low; j <= high; j++) {
        char found = 0;
        for (i = 0; i < type_counts[index]; i++) {
            if ((inst_params[index][j]->misc && factory_names[index][i]) && (0 == strcmp(inst_params[index][j]->misc, factory_names[index][i]))) found = 1;
        }
        if(found==0) {
            printf("Unknown type (%s) encountered!\n", inst_param[j]->misc);
            return -1;
        }
    }

    return 0;
}

void add_dependence (int fromtype, int totype, ocrMappable_t *frominstance, ocrParamList_t *fromparam, ocrMappable_t *toinstance, ocrParamList_t *toparam, int dependence_index, int dependence_count)
{

    switch(fromtype) {
    case 0:
    case 1:
    case 4:
    case 6:
        printf("Unexpected: this should have no dependences! (incorrect dependence: %s to %s)\n", fromparam->misc, toparam->misc);
        break;

    case 2: {
            ocrMemTarget_t *f = (ocrMemTarget_t *)frominstance;
            printf("Memtarget %s to %s\n", fromparam->misc, toparam->misc);

            if (f->memoryCount == 0) {
                f->memoryCount = dependence_count;
                f->memories = (ocrMemPlatform_t **)malloc(sizeof(ocrMemPlatform_t *) * dependence_count);
            }
            f->memories[dependence_index] = (ocrMemPlatform_t *)toinstance;
            break;
        }
    case 3: {
            printf("Allocator %s to %s\n", fromparam->misc, toparam->misc);
            ocrAllocator_t *f = (ocrAllocator_t *)frominstance;

            if (f->memoryCount == 0) {
                f->memoryCount = dependence_count;
                f->memories = (ocrMemTarget_t **)malloc(sizeof(ocrMemTarget_t *) * dependence_count);
            }
            f->memories[dependence_index] = (ocrMemTarget_t *)toinstance;
            break;
        }
    case 5: {
            ocrCompTarget_t *f = (ocrCompTarget_t *)frominstance;
            printf("CompTarget %s to %s\n", fromparam->misc, toparam->misc);

            if (f->platformCount == 0) {
                f->platformCount = dependence_count;
                f->platforms = (ocrCompPlatform_t **)malloc(sizeof(ocrCompPlatform_t *) * dependence_count);
            }
            f->platforms[dependence_index] = (ocrCompPlatform_t *)toinstance;
            break;
        }
    case 7: {
            ocrWorker_t *f = (ocrWorker_t *)frominstance;
            printf("Worker %s to %s\n", fromparam->misc, toparam->misc);

            if (f->computeCount == 0) {
                f->computeCount = dependence_count;
                f->computes = (ocrCompTarget_t **)malloc(sizeof(ocrCompTarget_t *) * dependence_count);
            }
            f->computes[dependence_index] = (ocrCompTarget_t *)toinstance;
            break;
        }
    case 8: {
            ocrScheduler_t *f = (ocrScheduler_t *)frominstance;
            printf("Scheduler %s to %s\n", fromparam->misc, toparam->misc);
            switch (totype) {
                case 6: {
                        if (f->workpileCount == 0) {
                            f->workpileCount = dependence_count;
                            f->workpiles = (ocrWorkpile_t **)malloc(sizeof(ocrWorkpile_t *) * dependence_count);
                        }
                        f->workpiles[dependence_index] = (ocrWorkpile_t *)toinstance;
                        break;
                    }
                case 7: {
                        if (f->workerCount == 0) {
                            f->workerCount = dependence_count;
                            f->workers = (ocrWorker_t **)malloc(sizeof(ocrWorker_t *) * dependence_count);
                        }
                        f->workers[dependence_index] = (ocrWorker_t *)toinstance;
                        break;
                    }
                default:
                        break;
            }
            break;
        }
    case 9: {
            ocrPolicyDomain_t *f = (ocrPolicyDomain_t *)frominstance;
            printf("PD %s to %s (%d)\n", fromparam->misc, toparam->misc, totype);
            switch (totype) {
                case 0: {
                        ASSERT(dependence_count==1);
                        /*	Special case handled during PD creation  */
                        break;
                    }
                case 2: {
                        if (f->memories == NULL) {
                            ASSERT(f->memoryCount == dependence_count);
                            f->memories = (ocrMemTarget_t **)malloc(sizeof(ocrMemTarget_t *) * dependence_count);
                        }
                        f->memories[dependence_index] = (ocrMemTarget_t *)toinstance;
                        break;
                    }
                case 3: {
                        if (f->allocators == NULL) {
                            ASSERT(f->allocatorCount == dependence_count);
                            f->allocators = (ocrAllocator_t **)malloc(sizeof(ocrAllocator_t *) * dependence_count);
                        }
                        f->allocators[dependence_index] = (ocrAllocator_t *)toinstance;
                        break;
                    }
                case 5: {
                        if (f->computes == NULL) {
                            ASSERT(f->computeCount == dependence_count);
                            f->computes = (ocrCompTarget_t **)malloc(sizeof(ocrCompTarget_t *) * dependence_count);
                        }
                        f->computes[dependence_index] = (ocrCompTarget_t *)toinstance;
                        break;
                    }
                case 6: {
                        if (f->workpiles == NULL) {
                            ASSERT(f->workpileCount == dependence_count);
                            f->workpiles = (ocrWorkpile_t **)malloc(sizeof(ocrWorkpile_t *) * dependence_count);
                        }
                        f->workpiles[dependence_index] = (ocrWorkpile_t *)toinstance;
                        break;
                    }
                case 7: {
                        if (f->workers == NULL) {
                            ASSERT(f->workerCount == dependence_count);
                            f->workers = (ocrWorker_t **)malloc(sizeof(ocrWorker_t *) * dependence_count);
                        }
                        f->workers[dependence_index] = (ocrWorker_t *)toinstance;
                        break;
                    }
                case 8: {
                        if (f->schedulers == NULL) {
                            ASSERT(f->schedulerCount == dependence_count);
                            f->schedulers = (ocrScheduler_t **)malloc(sizeof(ocrScheduler_t *) * dependence_count);
                        }
                        f->schedulers[dependence_index] = (ocrScheduler_t *)toinstance;
                        break;
                    }
                default:
                        break;
            }
            break;
        }
    default:
        break;
    }
}

int build_deps (dictionary *dict, int A, int B, char *refstr)
{
    int i, j, k;
    int low, high, count;
    int l, h;

    for (i = 0; i < iniparser_getnsec(dict); i++) {
        // Go thru the secnames looking for instances of A
        if (strncasecmp(inst_str[A], iniparser_getsecname(dict, i), strlen(inst_str[A]))==0) {
            // For each secname, look up corresponding instance of A through id
            count = read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
            for (j = low; j <= high; j++) {
                // Parse corresponding dependences from secname
                read_range(dict, iniparser_getsecname(dict, i), refstr, &l, &h);
                // Fetch corresponding instances of B
                for (k = l; k <= h; k++) {
                    // Connect A with B
                    // printf("%s, id %d has dependence on %s, id %d\n", inst_str[A], j, inst_str[B], k);
                    add_dependence(A, B, all_instances[A][j], inst_params[A][j], all_instances[B][k], inst_params[B][k], k-l, h-l+1);
                }
            }
        }
    }
    return 0;
}

/****************************************************************/

int main(int argc, char *argv[])
{
    char **seckey;
    int i, j, k, iter, count;
    int total_types;
    char *typestr;
    char key[64];
    dictionary *dict = iniparser_load(argv[1]);

    total_types = sizeof(type_str)/sizeof(const char *);

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
                factory_names[j][count] = populate_type(type_params[j], j, count, type_counts[j], dict, iniparser_getsecname(dict, i));
                // Create factories
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
                populate_inst(inst_params[j], all_instances[j], j, count++, inst_counts[j], dict, iniparser_getsecname(dict, i));
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
        build_deps(dict, deps[i].from, deps[i].to, deps[i].refstr);
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
    ocrFinalize();

    return 0;
}
