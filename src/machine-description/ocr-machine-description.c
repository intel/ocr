#include <stdio.h>
#include <string.h>
#include <iniparser.h>
#include <signal.h>
#include "ocr-machine.h"

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

#define INI_GET_BOOL(KEY, VAR, DEF) \
    VAR = (int) iniparser_getboolean(dict, KEY, DEF); \
    if (VAR==DEF) \
        printf("Key %s not found or invalid!\n", KEY); 

#define INI_GET_INT(KEY, VAR, DEF) \
    VAR = (int) iniparser_getint(dict, KEY, DEF); \
    if (VAR==DEF) \
        printf("Key %s not found or invalid!\n", KEY);

#define INI_GET_STR(KEY, VAR, DEF) \
    VAR = (char *) iniparser_getstring(dict, KEY, DEF); \
    if (!strcmp(VAR, DEF)) \
        printf("Key %s not found or invalid!\n", KEY);

#define TO_ENUM(VAR, STR, TYPE, TYPESTR, COUNT) \
    int kount; \
    for (kount=0; kount<COUNT; kount++) \
        if (!strcmp(STR, TYPESTR[kount])) \
            VAR=(TYPE)kount;

#define INI_IS_RANGE(KEY) \
    strstr((char *) iniparser_getstring(dict, KEY, ""), "-")

#define INI_GET_RANGE(KEY, LOW, HIGH) \
    sscanf(iniparser_getstring(dict, KEY, ""), "%d-%d", &LOW, &HIGH)

extern const char *inst_str[];

// TODO: expand to parse comma separated values & ranges iterating the below thru strtok with ,
// TODO: stretch goal, extend this to expressions: surely you're joking, Mr. Feynman
int read_range(dictionary *dict, char *sec, char *field, int *low, int *high) {
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

char* populate_type(ocrParamList_t *type_param, type_enum index, dictionary *dict, char *secname) {
    char *typestr;
    char key[64];

    // TODO: populate type-specific fields
    switch (index) {
    case guid_type:
        ALLOC_PARAM_LIST(type_param, paramListGuidProviderFact_t);
        break;
    case memplatform_type:
        ALLOC_PARAM_LIST(type_param, paramListMemPlatformFact_t);
        break;
    case memtarget_type:
        ALLOC_PARAM_LIST(type_param, paramListMemTargetFact_t);
        break;
    case allocator_type:
        ALLOC_PARAM_LIST(type_param, paramListAllocatorFact_t);
        break;
    case compplatform_type:
        ALLOC_PARAM_LIST(type_param, paramListCompPlatformFact_t);
        break;
    case comptarget_type:
        ALLOC_PARAM_LIST(type_param, paramListCompTargetFact_t);
        break;
    case workpile_type:
        ALLOC_PARAM_LIST(type_param, paramListWorkpileFact_t);
        break;
    case worker_type:
        ALLOC_PARAM_LIST(type_param, paramListWorkerFact_t);
        break;
    case scheduler_type:
        ALLOC_PARAM_LIST(type_param, paramListSchedulerFact_t);
        break;
    case policydomain_type:
        ALLOC_PARAM_LIST(type_param, paramListPolicyDomainFact_t);
        break;
    default:
        printf("Error: %d index unexpected\n", index);
        break;
    }

    snprintf(key, 64, "%s:%s", secname, "name");
    INI_GET_STR (key, typestr, "");
    return strdup(typestr);
}

ocrCompPlatformFactory_t *create_factory_compplatform(char *name, ocrParamList_t *paramlist) {
    compPlatformType_t mytype = -1;
    TO_ENUM (mytype, name, compPlatformType_t, compplatform_types, compPlatformMax_id);
  
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a compplatform factory of type %d: %s\n", mytype, factory_names[compplatform_type][mytype]); 
        return newCompPlatformFactory(mytype, paramlist);
    }
}

ocrMemPlatformFactory_t *create_factory_memplatform(char *name, ocrParamList_t *paramlist) {
    memPlatformType_t mytype = -1;
    TO_ENUM (mytype, name, memPlatformType_t, memplatform_types, memPlatformMax_id);
  
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a memplatform factory of type %d: %s\n", mytype, factory_names[memplatform_type][mytype]); 
        return newMemPlatformFactory(mytype, paramlist);
    }
}

ocrMemPlatformFactory_t *create_factory_memtarget(char *name, ocrParamList_t *paramlist) {
    memTargetType_t mytype = -1;
    TO_ENUM (mytype, name, memTargetType_t, memtarget_types, memTargetMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a memtarget factory of type %d: %s\n", mytype, factory_names[memtarget_type][mytype]); 
        return (ocrMemPlatformFactory_t *)newMemTargetFactory(mytype, paramlist);
    }
}

ocrAllocatorFactory_t *create_factory_allocator(char *name, ocrParamList_t *paramlist) {
    allocatorType_t mytype = -1;
    TO_ENUM (mytype, name, allocatorType_t, allocator_types, allocatorMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating an allocator factory of type %d: %s\n", mytype, factory_names[allocator_type][mytype]); 
        return (ocrAllocatorFactory_t *)newAllocatorFactory(mytype, paramlist);
    }
}

ocrCompTargetFactory_t *create_factory_comptarget(char *name, ocrParamList_t *paramlist) {
    compTargetType_t mytype = -1;
    TO_ENUM (mytype, name, compTargetType_t, comptarget_types, compTargetMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a comptarget factory of type %d: %s\n", mytype, factory_names[compptarget_type][mytype]); 
        return (ocrCompTargetFactory_t *)newCompTargetFactory(mytype, paramlist);
    }
}

ocrWorkpileFactory_t *create_factory_workpile(char *name, ocrParamList_t *paramlist) {
    workpileType_t mytype = -1;
    TO_ENUM (mytype, name, workpileType_t, workpile_types, workpileMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a workpile factory of type %d: %s\n", mytype, factory_names[workpile_type][mytype]); 
        return (ocrWorkpileFactory_t *)newWorkpileFactory(mytype, paramlist);
    }
}

ocrWorkerFactory_t *create_factory_worker(char *name, ocrParamList_t *paramlist) {
    workerType_t mytype = -1;
    TO_ENUM (mytype, name, workerType_t, worker_types, workerMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a worker factory of type %d: %s\n", mytype, factory_names[worker_type][mytype]); 
        return (ocrWorkerFactory_t *)newWorkerFactory(mytype, paramlist);
    }
}

ocrSchedulerFactory_t *create_factory_scheduler(char *name, ocrParamList_t *paramlist) {
    schedulerType_t mytype = -1;
    TO_ENUM (mytype, name, schedulerType_t, scheduler_types, schedulerMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a scheduler factory of type %d: %s\n", mytype, factory_names[scheduler_type][mytype]); 
        return (ocrSchedulerFactory_t *)newSchedulerFactory(mytype, paramlist);
    }
}

ocrPolicyDomainFactory_t *create_factory_policydomain(char *name, ocrParamList_t *paramlist) {
    policyDomainType_t mytype = -1;
    TO_ENUM (mytype, name, policyDomainType_t, policyDomain_types, policyDomainMax_id);
    if (mytype == -1) {
        printf("Unrecognized type %s\n", name);
        return NULL;
    } else { 
        //printf("Creating a worker factory of type %d: %s\n", mytype, factory_names[policydomain_type][mytype]); 
        return (ocrPolicyDomainFactory_t *)newPolicyDomainFactory(mytype, paramlist);
    }
}

ocrTaskFactory_t *create_factory_task(char *name, ocrParamList_t *paramlist) {
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

ocrTaskTemplateFactory_t *create_factory_tasktemplate(char *name, ocrParamList_t *paramlist) {
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

ocrDataBlockFactory_t *create_factory_datablock(char *name, ocrParamList_t *paramlist) {
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

ocrEventFactory_t *create_factory_event(char *name, ocrParamList_t *paramlist) {
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

ocrPolicyCtxFactory_t *create_factory_context(char *name, ocrParamList_t *paramlist) {
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

ocrGuidProviderFactory_t *create_factory_guid(char *name, ocrParamList_t *paramlist) {
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

ocrLockFactory_t *create_factory_lock(char *name, ocrParamList_t *paramlist) {
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

ocrAtomic64Factory_t *create_factory_atomic64(char *name, ocrParamList_t *paramlist) {
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

void *create_factory (type_enum index, char *factory_name, ocrParamList_t *paramlist) {
    void *new_factory;

    switch (index) {
    case guid_type:
        new_factory = (void *)create_factory_guid(factory_name, paramlist);
        break;
    case memplatform_type:
        new_factory = (void *)create_factory_memplatform(factory_name, paramlist);
        break;
    case memtarget_type:
        new_factory = (void *)create_factory_memtarget(factory_name, paramlist);
        break;
    case allocator_type:
        new_factory = (void *)create_factory_allocator(factory_name, paramlist);
        break;
    case compplatform_type:
        new_factory = (void *)create_factory_compplatform(factory_name, paramlist);
        break;
    case comptarget_type:
        new_factory = (void *)create_factory_comptarget(factory_name, paramlist);
        break;
    case workpile_type:
        new_factory = (void *)create_factory_workpile(factory_name, paramlist);
        break;
    case worker_type:
        new_factory = (void *)create_factory_worker(factory_name, paramlist);
        break;
    case scheduler_type:
        new_factory = (void *)create_factory_scheduler(factory_name, paramlist);
        break;
    case policydomain_type:
        new_factory = (void *)create_factory_policydomain(factory_name, paramlist);
        break;
    default:
        printf("Error: %d index unexpected\n", index);
        break;
    }
    return new_factory;
}

int populate_inst(ocrParamList_t **inst_param, ocrMappable_t **instance, int *type_counts, char ***factory_names, void ***all_factories, ocrMappable_t ***all_instances, type_enum index, dictionary *dict, char *secname) {
    int i, low, high, j;
    char *inststr;
    char key[64];
    void *factory;
    int value;

    read_range(dict, secname, "id", &low, &high); 

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
    case guid_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListGuidProviderInst_t);
            instance[j] = (ocrMappable_t *)((ocrGuidProviderFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created guid provider of type %s, index %d\n", inststr, j);
        }
        break;
    case memplatform_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemPlatformInst_t);
            instance[j] = (ocrMappable_t *)((ocrMemPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created memplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case memtarget_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemTargetInst_t);
            instance[j] = (ocrMappable_t *)((ocrMemTargetFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created memtarget of type %s, index %d\n", inststr, j);
        }
        break;
    case allocator_type:
        for (j = low; j<=high; j++) { 
            ALLOC_PARAM_LIST(inst_param[j], paramListAllocatorInst_t);
            snprintf(key, 64, "%s:%s", secname, "size");
            INI_GET_INT (key, value, -1);
            ((paramListAllocatorInst_t *)inst_param[j])->size = value;
            instance[j] = (ocrMappable_t *)((ocrAllocatorFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created allocator of type %s, index %d\n", inststr, j);
        }
        break;
    case compplatform_type:
        for (j = low; j<=high; j++) {
            // ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformInst_t);

            // FIXME: for now assume it's Pthread
            ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformPthread_t);
            snprintf(key, 64, "%s:%s", secname, "ismasterthread");
            INI_GET_BOOL (key, value, -1);
            ((paramListCompPlatformPthread_t *)inst_param[j])->isMasterThread = value;
            snprintf(key, 64, "%s:%s", secname, "stacksize");
            INI_GET_BOOL (key, value, -1);
            ((paramListCompPlatformPthread_t *)inst_param[j])->stackSize = value;

            instance[j] = (ocrMappable_t *)((ocrCompPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created compplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case comptarget_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListCompTargetInst_t);
            instance[j] = (ocrMappable_t *)((ocrCompTargetFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created comptarget of type %s, index %d\n", inststr, j);
        }
        break;
    case workpile_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListWorkpileInst_t);
            instance[j] = (ocrMappable_t *)((ocrWorkpileFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created workpile of type %s, index %d\n", inststr, j);
        }
        break;
    case worker_type:
        for (j = low; j<=high; j++) {
            //ALLOC_PARAM_LIST(inst_param[j], paramListWorkerInst_t);
            // FIXME: for now assume it's HC
            ALLOC_PARAM_LIST(inst_param[j], paramListWorkerHcInst_t);
            snprintf(key, 64, "%s:%s", secname, "workerid");
            INI_GET_INT (key, value, -1);
            ((paramListWorkerHcInst_t *)inst_param[j])->workerId = value; // TODO: just use id(j) rather than a separate key ?
            instance[j] = (ocrMappable_t *)((ocrWorkerFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created worker of type %s, index %d\n", inststr, j);
        }
        break;
    case scheduler_type:
        for (j = low; j<=high; j++) {
            //ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerInst_t);
            // FIXME: for now assume it's HC
            ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerHcInst_t);
            snprintf(key, 64, "%s:%s", secname, "workeridfirst");
            INI_GET_INT (key, value, -1);
            ((paramListSchedulerHcInst_t *)inst_param[j])->worker_id_first = value;
            instance[j] = (ocrMappable_t *)((ocrSchedulerFactory_t *)factory)->instantiate(factory, inst_param[j]);        
            if (instance[j]) printf("Created scheduler of type %s, index %d\n", inststr, j);
        }
        break;
    case policydomain_type:
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
            gf = (ocrGuidProvider_t *)(all_instances[0][low]); // Very special case: FIXME
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
        printf("Error: %d index unexpected\n", index);
        break;
    }
#if 0
    // Populate the params
    snprintf(key, 64, "%s:%s", secname, "type");
    INI_GET_STR (key, inststr, "");
    for (j = low; j <= high; j++) inst_param[j]->misc = strdup(inststr);

    // Sanity check that the type is already known
    for (j = low; j <= high; j++) {
        char found = 0;
        for (i = 0; i < type_counts[index]; i++) {
            if ((inst_param[j]->misc && factory_names[index][i]) && (0 == strcmp(inst_param[j]->misc, factory_names[index][i]))) found = 1;
        }
        if(found==0) {
            printf("Unknown type (%s) encountered!\n", inst_param[j]->misc);
            return -1;
        }
    }
#endif
    return 0;
}

void add_dependence (type_enum fromtype, type_enum totype, ocrMappable_t *frominstance, ocrParamList_t *fromparam, ocrMappable_t *toinstance, ocrParamList_t *toparam, int dependence_index, int dependence_count) {
    switch(fromtype) {
    case guid_type:
    case memplatform_type:
    case compplatform_type:
    case workpile_type:
        printf("Unexpected: this should have no dependences! (incorrect dependence: %d to %d)\n", fromtype, totype);
        break;

    case memtarget_type: {
            ocrMemTarget_t *f = (ocrMemTarget_t *)frominstance;
            // printf("Memtarget %d to %d\n", fromtype, totype);

            if (f->memoryCount == 0) {
                f->memoryCount = dependence_count;
                f->memories = (ocrMemPlatform_t **)malloc(sizeof(ocrMemPlatform_t *) * dependence_count);
            }
            f->memories[dependence_index] = (ocrMemPlatform_t *)toinstance;
            break;
        }
    case allocator_type: {
            // printf("Allocator %d to %d\n", fromtype, totype);
            ocrAllocator_t *f = (ocrAllocator_t *)frominstance;

            if (f->memoryCount == 0) {
                f->memoryCount = dependence_count;
                f->memories = (ocrMemTarget_t **)malloc(sizeof(ocrMemTarget_t *) * dependence_count);
            }
            f->memories[dependence_index] = (ocrMemTarget_t *)toinstance;
            break;
        }
    case comptarget_type: {
            ocrCompTarget_t *f = (ocrCompTarget_t *)frominstance;
            // printf("CompTarget %d to %d\n", fromtype, totype);

            if (f->platformCount == 0) {
                f->platformCount = dependence_count;
                f->platforms = (ocrCompPlatform_t **)malloc(sizeof(ocrCompPlatform_t *) * dependence_count);
            }
            f->platforms[dependence_index] = (ocrCompPlatform_t *)toinstance;
            break;
        }
    case worker_type: {
            ocrWorker_t *f = (ocrWorker_t *)frominstance;
            // printf("Worker %d to %d\n", fromtype, totype);

            if (f->computeCount == 0) {
                f->computeCount = dependence_count;
                f->computes = (ocrCompTarget_t **)malloc(sizeof(ocrCompTarget_t *) * dependence_count);
            }
            f->computes[dependence_index] = (ocrCompTarget_t *)toinstance;
            break;
        }
    case scheduler_type: {
            ocrScheduler_t *f = (ocrScheduler_t *)frominstance;
            // printf("Scheduler %d to %d\n", fromtype, totype);
            switch (totype) {
                case workpile_type: {
                        if (f->workpileCount == 0) {
                            f->workpileCount = dependence_count;
                            f->workpiles = (ocrWorkpile_t **)malloc(sizeof(ocrWorkpile_t *) * dependence_count);
                        }
                        f->workpiles[dependence_index] = (ocrWorkpile_t *)toinstance;
                        break;
                    }
                case worker_type: {
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
    case policydomain_type: {
            ocrPolicyDomain_t *f = (ocrPolicyDomain_t *)frominstance;
            // printf("PD %d to %d\n", fromtype, totype);
            switch (totype) {
                case guid_type: {
                        ASSERT(dependence_count==1);
                        /*	Special case handled during PD creation  */
                        break;
                    }
                case memtarget_type: {
                        if (f->memories == NULL) {
                            ASSERT(f->memoryCount == dependence_count);
                            f->memories = (ocrMemTarget_t **)malloc(sizeof(ocrMemTarget_t *) * dependence_count);
                        }
                        f->memories[dependence_index] = (ocrMemTarget_t *)toinstance;
                        break;
                    }
                case allocator_type: {
                        if (f->allocators == NULL) {
                            ASSERT(f->allocatorCount == dependence_count);
                            f->allocators = (ocrAllocator_t **)malloc(sizeof(ocrAllocator_t *) * dependence_count);
                        }
                        f->allocators[dependence_index] = (ocrAllocator_t *)toinstance;
                        break;
                    }
                case comptarget_type: {
                        if (f->computes == NULL) {
                            ASSERT(f->computeCount == dependence_count);
                            f->computes = (ocrCompTarget_t **)malloc(sizeof(ocrCompTarget_t *) * dependence_count);
                        }
                        f->computes[dependence_index] = (ocrCompTarget_t *)toinstance;
                        break;
                    }
                case workpile_type: {
                        if (f->workpiles == NULL) {
                            ASSERT(f->workpileCount == dependence_count);
                            f->workpiles = (ocrWorkpile_t **)malloc(sizeof(ocrWorkpile_t *) * dependence_count);
                        }
                        f->workpiles[dependence_index] = (ocrWorkpile_t *)toinstance;
                        break;
                    }
                case worker_type: {
                        if (f->workers == NULL) {
                            ASSERT(f->workerCount == dependence_count);
                            f->workers = (ocrWorker_t **)malloc(sizeof(ocrWorker_t *) * dependence_count);
                        }
                        f->workers[dependence_index] = (ocrWorker_t *)toinstance;
                        break;
                    }
                case scheduler_type: {
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

int build_deps (dictionary *dict, int A, int B, char *refstr, ocrMappable_t ***all_instances, ocrParamList_t ***inst_params) {
    int i, j, k;
    int low, high;
    int l, h;

    for (i = 0; i < iniparser_getnsec(dict); i++) {
        // Go thru the secnames looking for instances of A
        if (strncasecmp(inst_str[A], iniparser_getsecname(dict, i), strlen(inst_str[A]))==0) { 
            // For each secname, look up corresponding instance of A through id
            read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
            for (j = low; j <= high; j++) {
                // Parse corresponding dependences from secname
                read_range(dict, iniparser_getsecname(dict, i), refstr, &l, &h);
                // Fetch corresponding instances of B
                for (k = l; k <= h; k++) {
                    // Connect A with B 
                    add_dependence(A, B, all_instances[A][j], inst_params[A][j], all_instances[B][k], inst_params[B][k], k-l, h-l+1);
                }
            }
        }
    }
    return 0;
}
