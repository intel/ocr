/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */


#include "allocator/allocator-all.h"
#include "comp-platform/comp-platform-all.h"
#include "comp-target/comp-target-all.h"
#include "datablock/datablock-all.h"
#include "debug.h"
#include "event/event-all.h"
#include "external/iniparser.h"
#include "guid/guid-all.h"
#include "machine-description/ocr-machine.h"
#include "mem-platform/mem-platform-all.h"
#include "mem-target/mem-target-all.h"
#include "ocr-types.h"
#include "policy-domain/policy-domain-all.h"
#include "scheduler/scheduler-all.h"
#ifdef OCR_ENABLE_STATISTICS
#include "statistics/statistics-all.h"
#endif
//#include "sync/sync-all.h"
#include "task/task-all.h"
#include "worker/worker-all.h"
#include "workpile/workpile-all.h"

#include "ocr-sysboot.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>


#define MAX_KEY_SZ 64
#define DEBUG_TYPE INIPARSING

#define INI_GET_BOOL(KEY, VAR, DEF) \
    VAR = (s32) iniparser_getboolean(dict, KEY, DEF); \
    if (VAR==DEF) { \
        DPRINTF(DEBUG_LVL_WARN, "Key %s not found or invalid!\n", KEY); \
    }

#define INI_GET_INT(KEY, VAR, DEF) \
    VAR = (s32) iniparser_getint(dict, KEY, DEF); \
    if (VAR==DEF) {\
        DPRINTF(DEBUG_LVL_WARN, "Key %s not found or invalid!\n", KEY); \
    }

#define INI_GET_STR(KEY, VAR, DEF) \
    VAR = (char *) iniparser_getstring(dict, KEY, DEF); \
    if (!strcmp(VAR, DEF)) {\
        DPRINTF(DEBUG_LVL_WARN, "Key %s not found or invalid!\n", KEY); \
    }

#define TO_ENUM(VAR, STR, TYPE, TYPESTR, COUNT) \
    s32 kount; \
    for (kount=0; kount<COUNT; kount++) \
        if (!strcmp(STR, TYPESTR[kount])) \
            VAR=(TYPE)kount;

#define INI_IS_RANGE(KEY) \
    strstr((char *) iniparser_getstring(dict, KEY, ""), "-")

#define INI_IS_CSV(KEY) \
    strstr((char *) iniparser_getstring(dict, KEY, ""), ",")

#define INI_GET_RANGE(KEY, LOW, HIGH) \
    sscanf(iniparser_getstring(dict, KEY, ""), "%d-%d", &LOW, &HIGH)

typedef enum value_type_t {
    TYPE_UNKNOWN,
    TYPE_CSV,
    TYPE_RANGE,
    TYPE_INT
} value_type;

extern const char *inst_str[];

bool key_exists(dictionary *dict, char *sec, char *field) {
    char key[MAX_KEY_SZ];
    
    snprintf(key, MAX_KEY_SZ, "%s:%s", sec, field);
    if(iniparser_getstring(dict, key, NULL) != NULL)
        return true;
    else
        return false;
}

// TODO: expand to parse comma separated values & ranges iterating the below thru strtok with ,
// TODO: stretch goal, extend this to expressions: surely you're joking, Mr. Feynman
s32 read_range(dictionary *dict, char *sec, char *field, s32 *low, s32 *high) {
    char key[MAX_KEY_SZ];
    s32 value;
    s32 lo, hi;
    s32 count;

    snprintf(key, MAX_KEY_SZ, "%s:%s", sec, field);
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

/*  This function, when called with the section name & field, gives the first token, followed by subsequent calls that give other ones */
s32 read_next_csv_value(dictionary *dict, char *key) {
    static char *parsestr = NULL; // This gives the current string that's parsed
    static char *currentfield = NULL;
    static char *saveptr = NULL;

    if ((parsestr != NULL) && (!strcmp(currentfield, iniparser_getstring(dict, key, "")))) {
        parsestr = strtok_r(NULL, ",", &saveptr);
    } else {
        currentfield = iniparser_getstring(dict, key, "");
        parsestr = strtok_r(currentfield, ",", &saveptr);
    }

    if (parsestr == NULL) return -1;
    else return atoi(parsestr); 
}

s32 get_key_value(dictionary *dict, char *sec, char *field, s32 offset) {
    char key[MAX_KEY_SZ];
    s32 lo, hi;
    s32 retval;
    static value_type key_value_type = TYPE_UNKNOWN;

    snprintf(key, MAX_KEY_SZ, "%s:%s", sec, field);
    if (key_value_type == TYPE_UNKNOWN) {
        if (INI_IS_CSV(key)) {
            key_value_type = TYPE_CSV;
        } else if (INI_IS_RANGE(key)) {
            key_value_type = TYPE_RANGE;
        } else {
            key_value_type = TYPE_INT; // Big assumption, could break
        }
    }

    if (key_value_type == TYPE_CSV) {
        retval = read_next_csv_value(dict, key);
        if (retval == -1) key_value_type = TYPE_UNKNOWN;        
    } else {
        read_range(dict, sec, field, &lo, &hi);
        retval = lo+offset;
        key_value_type = TYPE_UNKNOWN;        
    }

    return retval;
}

char* populate_type(ocrParamList_t **type_param, type_enum index, dictionary *dict, char *secname) {
    char *typestr;
    char key[MAX_KEY_SZ];
    int value = 0;

    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "name");
    INI_GET_STR (key, typestr, "");

    // TODO: populate type-specific fields as-needed; see compplatform_type for an example
    switch (index) {
    case guid_type:
        ALLOC_PARAM_LIST(*type_param, paramListGuidProviderFact_t);
        break;
    case memplatform_type:
        ALLOC_PARAM_LIST(*type_param, paramListMemPlatformFact_t);
        break;
    case memtarget_type:
        ALLOC_PARAM_LIST(*type_param, paramListMemTargetFact_t);
        break;
    case allocator_type:
        ALLOC_PARAM_LIST(*type_param, paramListAllocatorFact_t);
        break;
    case compplatform_type: {
            compPlatformType_t mytype = -1;
            TO_ENUM (mytype, typestr, compPlatformType_t, compplatform_types, compPlatformMax_id);
            switch (mytype) {
                case compPlatformPthread_id: {
                    ALLOC_PARAM_LIST(*type_param, paramListCompPlatformPthread_t);
                    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "stacksize");
                    INI_GET_INT (key, value, -1);
                    ((paramListCompPlatformPthread_t *)(*type_param))->stackSize = (value==-1)?0:value;
                }
                break;
                default:
                    ALLOC_PARAM_LIST(*type_param, paramListCompPlatformFact_t);
                break;
            }
        }
        break;
    case comptarget_type:
        ALLOC_PARAM_LIST(*type_param, paramListCompTargetFact_t);
        break;
    case workpile_type:
        ALLOC_PARAM_LIST(*type_param, paramListWorkpileFact_t);
        break;
    case worker_type:
        ALLOC_PARAM_LIST(*type_param, paramListWorkerFact_t);
        break;
    case scheduler_type:
        ALLOC_PARAM_LIST(*type_param, paramListSchedulerFact_t);
        break;
    case policydomain_type:
        ALLOC_PARAM_LIST(*type_param, paramListPolicyDomainFact_t);
        break;
    default:
        DPRINTF(DEBUG_LVL_WARN, "Error: %d index unexpected\n", index);
        break;
    }

    return strdup(typestr);
}

ocrCompPlatformFactory_t *create_factory_compplatform (char *name, ocrParamList_t *paramlist) {
    compPlatformType_t mytype = compPlatformMax_id;
    TO_ENUM (mytype, name, compPlatformType_t, compplatform_types, compPlatformMax_id);
    if (mytype == compPlatformMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a compplatform factory of type %d\n", mytype);
        return newCompPlatformFactory(mytype, paramlist);
    }
}

ocrMemPlatformFactory_t *create_factory_memplatform (char *name, ocrParamList_t *paramlist) {
    memPlatformType_t mytype = memPlatformMax_id;
    TO_ENUM (mytype, name, memPlatformType_t, memplatform_types, memPlatformMax_id);

    if (mytype == memPlatformMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a memplatform factory of type %d\n", mytype);
        return newMemPlatformFactory(mytype, paramlist);
    }
}

ocrMemPlatformFactory_t *create_factory_memtarget(char *name, ocrParamList_t *paramlist) {
    memTargetType_t mytype = memTargetMax_id;
    TO_ENUM (mytype, name, memTargetType_t, memtarget_types, memTargetMax_id);
    if (mytype == memTargetMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a memtarget factory of type %d\n", mytype);
        return (ocrMemPlatformFactory_t *)newMemTargetFactory(mytype, paramlist);
    }
}

ocrAllocatorFactory_t *create_factory_allocator(char *name, ocrParamList_t *paramlist) {
    allocatorType_t mytype = allocatorMax_id;
    TO_ENUM (mytype, name, allocatorType_t, allocator_types, allocatorMax_id);
    if (mytype == allocatorMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating an allocator factory of type %d\n", mytype);
        return (ocrAllocatorFactory_t *)newAllocatorFactory(mytype, paramlist);
    }
}

ocrCompTargetFactory_t *create_factory_comptarget(char *name, ocrParamList_t *paramlist) {
    compTargetType_t mytype = compTargetMax_id;
    TO_ENUM (mytype, name, compTargetType_t, comptarget_types, compTargetMax_id);
    if (mytype == compTargetMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a comptarget factory of type %d\n", mytype);
        return (ocrCompTargetFactory_t *)newCompTargetFactory(mytype, paramlist);
    }
}

ocrWorkpileFactory_t *create_factory_workpile(char *name, ocrParamList_t *paramlist) {
    workpileType_t mytype = workpileMax_id;
    TO_ENUM (mytype, name, workpileType_t, workpile_types, workpileMax_id);
    if (mytype == workpileMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a workpile factory of type %d\n", mytype);
        return (ocrWorkpileFactory_t *)newWorkpileFactory(mytype, paramlist);
    }
}

ocrWorkerFactory_t *create_factory_worker(char *name, ocrParamList_t *paramlist) {
    workerType_t mytype = workerMax_id;
    TO_ENUM (mytype, name, workerType_t, worker_types, workerMax_id);
    if (mytype == workerMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a worker factory of type %d\n", mytype);
        return (ocrWorkerFactory_t *)newWorkerFactory(mytype, paramlist);
    }
}

ocrSchedulerFactory_t *create_factory_scheduler(char *name, ocrParamList_t *paramlist) {
    schedulerType_t mytype = schedulerMax_id;
    TO_ENUM (mytype, name, schedulerType_t, scheduler_types, schedulerMax_id);
    if (mytype == schedulerMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a scheduler factory of type %d\n", mytype);
        return (ocrSchedulerFactory_t *)newSchedulerFactory(mytype, paramlist);
    }
}

ocrPolicyDomainFactory_t *create_factory_policydomain(char *name, ocrParamList_t *paramlist) {
    policyDomainType_t mytype = policyDomainMax_id;
    TO_ENUM (mytype, name, policyDomainType_t, policyDomain_types, policyDomainMax_id);
    if (mytype == policyDomainMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a worker factory of type %d\n", mytype);
        return (ocrPolicyDomainFactory_t *)newPolicyDomainFactory(mytype, paramlist);
    }
}

ocrTaskFactory_t *create_factory_task(char *name, ocrParamList_t *paramlist) {
    taskType_t mytype = taskMax_id;
    TO_ENUM (mytype, name, taskType_t, task_types, taskMax_id);
    if (mytype == taskMax_id) {
        DPRINTF(DEBUG_LVL_INFO, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a task factory of type %d\n", mytype);
        return (ocrTaskFactory_t *)newTaskFactory(mytype, paramlist);
    }
}

ocrTaskTemplateFactory_t *create_factory_tasktemplate(char *name, ocrParamList_t *paramlist) {
    taskTemplateType_t mytype = taskTemplateMax_id;
    TO_ENUM (mytype, name, taskTemplateType_t, taskTemplate_types, taskTemplateMax_id);
    if (mytype == taskTemplateMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a task template factory of type %d\n", mytype);
        return (ocrTaskTemplateFactory_t *)newTaskTemplateFactory(mytype, paramlist);
    }
}

ocrDataBlockFactory_t *create_factory_datablock(char *name, ocrParamList_t *paramlist) {
    dataBlockType_t mytype = dataBlockMax_id;
    TO_ENUM (mytype, name, dataBlockType_t, dataBlock_types, dataBlockMax_id);
    if (mytype == dataBlockMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a datablock factory of type %d\n", mytype);
        return (ocrDataBlockFactory_t *)newDataBlockFactory(mytype, paramlist);
    }
}

ocrEventFactory_t *create_factory_event(char *name, ocrParamList_t *paramlist) {
    eventType_t mytype = eventMax_id;
    TO_ENUM (mytype, name, eventType_t, event_types, eventMax_id);
    if (mytype == eventMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating an event factory of type %d\n", mytype);
        return (ocrEventFactory_t *)newEventFactory(mytype, paramlist);
    }
}

/*
ocrPolicyCtxFactory_t *create_factory_context(char *name, ocrParamList_t *paramlist) {
    policyCtxType_t mytype = policyCtxMax_id;
    TO_ENUM (mytype, name, policyCtxType_t, policyCtx_types, policyCtxMax_id);
    if (mytype == policyCtxMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a policy ctx factory of type %d\n", mytype);
        return (ocrPolicyCtxFactory_t *)newPolicyCtxFactory(mytype, paramlist);
    }
}
*/

ocrGuidProviderFactory_t *create_factory_guid(char *name, ocrParamList_t *paramlist) {
    guidType_t mytype = guidMax_id;
    TO_ENUM (mytype, name, guidType_t, guid_types, guidMax_id);
    if (mytype == guidMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a guid factory of type %d\n", mytype);
        return (ocrGuidProviderFactory_t *)newGuidProviderFactory(mytype, paramlist);
    }
}

/*
ocrLockFactory_t *create_factory_lock(char *name, ocrParamList_t *paramlist) {
    syncType_t mytype = syncMax_id;
    TO_ENUM (mytype, name, syncType_t, sync_types, syncMax_id);
    if (mytype == syncMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a lock factory of type %d\n", mytype);
        return (ocrLockFactory_t *)newLockFactory(mytype, paramlist);
    }
}

ocrAtomic64Factory_t *create_factory_atomic64(char *name, ocrParamList_t *paramlist) {
    syncType_t mytype = syncMax_id;
    TO_ENUM (mytype, name, syncType_t, sync_types, syncMax_id);
    if (mytype == syncMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating an atomic64 factory of type %d\n", mytype);
        return (ocrAtomic64Factory_t *)newAtomic64Factory(mytype, paramlist);
    }
}

ocrQueueFactory_t *create_factory_queue(char *name, ocrParamList_t *paramlist) {
    syncType_t mytype = -1;
    TO_ENUM (mytype, name, syncType_t, sync_types, syncMax_id);
    if (mytype == -1) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a queue factory of type %d\n", mytype);
        // HACK: FIXME: Passing static size
        return (ocrQueueFactory_t *)newQueueFactory(mytype, 32, paramlist);
    }
}
*/


void *create_factory (type_enum index, char *factory_name, ocrParamList_t *paramlist) {
    void *new_factory = NULL;

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
        DPRINTF(DEBUG_LVL_WARN, "Error: %d index unexpected\n", index);
        break;
    }
    return new_factory;
}

s32 populate_inst(ocrParamList_t **inst_param, void **instance, s32 *type_counts, char ***factory_names, void ***all_factories, void ***all_instances, type_enum index, dictionary *dict, char *secname) {
    s32 i, low, high, j;
    char *inststr;
    char key[MAX_KEY_SZ];
    void *factory;
    s32 value;

    read_range(dict, secname, "id", &low, &high);

    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "type");
    INI_GET_STR (key, inststr, "");

    for (i = 0; i < type_counts[index]; i++) {
        if (factory_names[index][i] && (0 == strncmp(factory_names[index][i], inststr, strlen(factory_names[index][i])))) break;
    }
    if (factory_names[index][i] == NULL || strncmp(factory_names[index][i], inststr, strlen(factory_names[index][i]))) {
        DPRINTF(DEBUG_LVL_WARN, "Unknown type %s\n", inststr);
        return 0;
    }

    // find factory based on i
    factory = all_factories[index][i];

    // Use the factory's instantiate() to create instance

    switch (index) {
    case guid_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListGuidProviderInst_t);
            instance[j] = (void *)((ocrGuidProviderFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created guid provider of type %s, index %d\n", inststr, j);
        }
        break;
    case memplatform_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemPlatformInst_t);
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "size");
            INI_GET_INT(key, value, -1);
            instance[j] = (void *)((ocrMemPlatformFactory_t *)factory)->instantiate(factory, 0, value, inst_param[j]);
//            instance[j] = (void *)((ocrMemPlatformFactory_t *)factory)->instantiate(factory, value, inst_param[j]);
/*
ocrMemPlatform_t* newMemPlatformMalloc(ocrMemPlatformFactory_t * factory,
                                       ocrPhysicalLocation_t location,
                                       u64 memSize, ocrParamList_t *perInstance) {
*/
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created memplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case memtarget_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemTargetInst_t);
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "size");
            INI_GET_INT(key, value, -1);
            instance[j] = (void *)((ocrMemTargetFactory_t *)factory)->instantiate(factory, 0, value, inst_param[j]);
//            instance[j] = (void *)((ocrMemTargetFactory_t *)factory)->instantiate(factory, value, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created memtarget of type %s, index %d\n", inststr, j);
        }
        break;
    case allocator_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListAllocatorInst_t);
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "size");
            INI_GET_INT (key, value, -1);
            ((paramListAllocatorInst_t *)inst_param[j])->size = value;
            instance[j] = (void *)((ocrAllocatorFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created allocator of type %s, index %d\n", inststr, j);
        }
        break;
    case compplatform_type:
        for (j = low; j<=high; j++) {

            compPlatformType_t mytype = -1;
            
            TO_ENUM (mytype, inststr, compPlatformType_t, compplatform_types, compPlatformMax_id);
            switch (mytype) {
                case compPlatformPthread_id: {
                    ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformPthread_t);
                    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "ismasterthread");
                    INI_GET_BOOL (key, value, -1);
                    ((paramListCompPlatformPthread_t *)inst_param[j])->isMasterThread = value;
                    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "stacksize");
                    INI_GET_INT (key, value, -1);
                    ((paramListCompPlatformPthread_t *)inst_param[j])->stackSize = (value==-1)?0:value;
                    if (key_exists(dict, secname, "binding")) {
                        value = get_key_value(dict, secname, "binding", j-low);
                        ((paramListCompPlatformPthread_t *)inst_param[j])->binding = value;
                    } else {
                        ((paramListCompPlatformPthread_t *)inst_param[j])->binding = -1;
                    }
                }
                break;
                default:
                    ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformInst_t);
                break;
            }

            instance[j] = (void *)((ocrCompPlatformFactory_t *)factory)->instantiate(factory, 0, inst_param[j]);
//            instance[j] = (void *)((ocrCompPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created compplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case comptarget_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListCompTargetInst_t);
            instance[j] = (void *)((ocrCompTargetFactory_t *)factory)->instantiate(factory, 0, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created comptarget of type %s, index %d\n", inststr, j);
        }
        break;
    case workpile_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListWorkpileInst_t);
            instance[j] = (void *)((ocrWorkpileFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created workpile of type %s, index %d\n", inststr, j);
        }
        break;
    case worker_type:
        for (j = low; j<=high; j++) {

            workerType_t mytype = -1;
            TO_ENUM (mytype, inststr, workerType_t, worker_types, workerMax_id);
            switch (mytype) {
                case workerHc_id: {
                    ALLOC_PARAM_LIST(inst_param[j], paramListWorkerHcInst_t);
                    ((paramListWorkerHcInst_t *)inst_param[j])->workerId = j; // using "id" for now; TODO: decide if a separate key is needed
                }
                break;
                default:
                    ALLOC_PARAM_LIST(inst_param[j], paramListWorkerInst_t);
                break;
            }

            instance[j] = (void *)((ocrWorkerFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created worker of type %s, index %d\n", inststr, j);
        }
        break;
    case scheduler_type:
        for (j = low; j<=high; j++) {
            schedulerType_t mytype = -1;
            TO_ENUM (mytype, inststr, schedulerType_t, scheduler_types, schedulerMax_id);
            switch (mytype) {
                case schedulerHc_id: {
                    ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerHcInst_t);
                    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "workeridfirst");
                    INI_GET_INT (key, value, -1);
                    ((paramListSchedulerHcInst_t *)inst_param[j])->workerIdFirst = value;
                }
                break;
                default:
                    ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerInst_t);
                break;
            }

            instance[j] = (void *)((ocrSchedulerFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created scheduler of type %s, index %d\n", inststr, j);
        }
        break;
    case policydomain_type:
        for (j = low; j<=high; j++) {
            ocrTaskFactory_t *tf;
            ocrTaskTemplateFactory_t *ttf;
            ocrDataBlockFactory_t *dbf;
            ocrEventFactory_t *ef;
            //ocrPolicyCtxFactory_t *cf;
            ocrGuidProvider_t *gf;
            //ocrLockFactory_t *lf;
            //ocrAtomic64Factory_t *af;
            //ocrQueueFactory_t *qf;
            s32 low, high;

            s32 schedulerCount, workerCount, allocatorCount;
            schedulerCount = read_range(dict, secname, "scheduler", &low, &high);
            workerCount = read_range(dict, secname, "worker", &low, &high);
//            computeCount = read_range(dict, secname, "comptarget", &low, &high);
//            workpileCount = read_range(dict, secname, "workpile", &low, &high);
            allocatorCount = read_range(dict, secname, "allocator", &low, &high);
//            memoryCount = read_range(dict, secname, "memtarget", &low, &high);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "taskfactory");
            INI_GET_STR (key, inststr, "");
            tf = create_factory_task(inststr, NULL);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "tasktemplatefactory");
            INI_GET_STR (key, inststr, "");
            ttf = create_factory_tasktemplate(inststr, NULL);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "datablockfactory");
            INI_GET_STR (key, inststr, "");
            dbf = create_factory_datablock(inststr, NULL);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "eventfactory");
            INI_GET_STR (key, inststr, "");
            ef = create_factory_event(inststr, NULL);
/*
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "contextfactory");
            INI_GET_STR (key, inststr, "");
            cf = create_factory_context(inststr, NULL);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "sync");
            INI_GET_STR (key, inststr, "");
            lf = create_factory_lock(inststr, NULL);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "sync");
            INI_GET_STR (key, inststr, "");
            af = create_factory_atomic64(inststr, NULL);

            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "sync");
            INI_GET_STR (key, inststr, "");
            qf = create_factory_queue(inststr, NULL);
*/
            // Ugly but special case
            read_range(dict, secname, "guid", &low, &high);
            ASSERT (low == high);  // We don't expect more than one guid provider
            gf = (ocrGuidProvider_t *)(all_instances[guid_type][low]);
            ASSERT (gf);

#ifdef OCR_ENABLE_STATISTICS
            // HUGE HUGE HUGE HUGE HUGE HACK. This needs to be parsed
            // but for now just passing some default one
            ocrStatsFactory_t *sf = newStatsFactory(statisticsDefault_id, NULL);
#endif


            ALLOC_PARAM_LIST(inst_param[j], paramListPolicyDomainInst_t);
            instance[j] = (void *)((ocrPolicyDomainFactory_t *)factory)->instantiate(
                factory, schedulerCount, allocatorCount, workerCount,
                tf, ttf, dbf, ef, gf, NULL, NULL, 
                inst_param[j]);

            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created policy domain of index %d\n", j);

//            setBootPD((ocrPolicyDomain_t *)instance[j]);
        }
        break;
    default:
        DPRINTF(DEBUG_LVL_WARN, "Error: %d index unexpected\n", index);
        break;
    }
    return 0;
}

void free_instance (void *instance, type_enum mytype)
{
    switch (mytype) {
    case memtarget_type: {
        ocrMemTarget_t *t = (ocrMemTarget_t *)instance;
        free(t->memories);
        break;
    }
    case allocator_type: {
        ocrAllocator_t *t = (ocrAllocator_t *)instance;
        free(t->memories);
        break;
    }
    case comptarget_type: {
        ocrCompTarget_t *t = (ocrCompTarget_t *)instance;
        free(t->platforms);
        break;
    }
    case worker_type: {
        ocrWorker_t *t = (ocrWorker_t *)instance;
        free(t->computes);
        break;
    }
    case scheduler_type: {
        ocrScheduler_t *t = (ocrScheduler_t *)instance;
        free(t->workpiles);
//        free(t->workers);
        break;
    }
    case policydomain_type: {
        ocrPolicyDomain_t *t = (ocrPolicyDomain_t *)instance;
//        free(t->memories);
//        free(t->computes);
//        free(t->workpiles);
        free(t->workers);
        free(t->allocators);
        free(t->schedulers);
        break;
    }
    case guid_type:
    case memplatform_type:
    case compplatform_type:
    case workpile_type:
    default:
        break;
    }
}

void add_dependence (type_enum fromtype, type_enum totype, void *frominstance, ocrParamList_t *fromparam, void *toinstance, ocrParamList_t *toparam, s32 dependence_index, s32 dependence_count) {
    switch(fromtype) {
    case guid_type:
    case memplatform_type:
    case compplatform_type:
    case workpile_type:
        DPRINTF(DEBUG_LVL_WARN, "Unexpected: this should have no dependences! (incorrect dependence: %d to %d)\n", fromtype, totype);
        break;

    case memtarget_type: {
        ocrMemTarget_t *f = (ocrMemTarget_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "Memtarget %d to %d\n", fromtype, totype);

        if (f->memoryCount == 0) {
            f->memoryCount = dependence_count;
            f->memories = (ocrMemPlatform_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrMemPlatform_t *), NULL);
        }
        f->memories[dependence_index] = (ocrMemPlatform_t *)toinstance;
        break;
    }
    case allocator_type: {
        ocrAllocator_t *f = (ocrAllocator_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "Allocator %d to %d\n", fromtype, totype);
        if (f->memoryCount == 0) {
            f->memoryCount = dependence_count;
            f->memories = (ocrMemTarget_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrMemTarget_t *), NULL);
        }
        f->memories[dependence_index] = (ocrMemTarget_t *)toinstance;
        break;
    }
    case comptarget_type: {
        ocrCompTarget_t *f = (ocrCompTarget_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "CompTarget %d to %d\n", fromtype, totype);

        if (f->platformCount == 0) {
            f->platformCount = dependence_count;
            f->platforms = (ocrCompPlatform_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrCompPlatform_t *),  NULL);
        }
        f->platforms[dependence_index] = (ocrCompPlatform_t *)toinstance;
        break;
    }
    case worker_type: {
        ocrWorker_t *f = (ocrWorker_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "Worker %d to %d\n", fromtype, totype);

        if (f->computeCount == 0) {
            f->computeCount = dependence_count;
            f->computes = (ocrCompTarget_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrCompTarget_t *), NULL);
        }
        f->computes[dependence_index] = (ocrCompTarget_t *)toinstance;
        break;
    }
    case scheduler_type: {
        ocrScheduler_t *f = (ocrScheduler_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "Scheduler %d to %d\n", fromtype, totype);
        switch (totype) {
        case workpile_type: {
            if (f->workpileCount == 0) {
                f->workpileCount = dependence_count;
                f->workpiles = (ocrWorkpile_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrWorkpile_t *), NULL);
            }
            f->workpiles[dependence_index] = (ocrWorkpile_t *)toinstance;
            break;
        }
/*        case worker_type: {
            if (f->workerCount == 0) {
                f->workerCount = dependence_count;
                f->workers = (ocrWorker_t **)runtimeChunkAlloc(dependence_count, sizeof(ocrWorker_t *));
            }
            f->workers[dependence_index] = (ocrWorker_t *)toinstance;
            break;
        }
*/        default:
            break;
        }
        break;
    }
    case policydomain_type: {
        ocrPolicyDomain_t *f = (ocrPolicyDomain_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "PD %d to %d\n", fromtype, totype);
        switch (totype) {
        case guid_type: {
            ASSERT(dependence_count==1);
            /*	Special case handled during PD creation  */
            break;
        }
        case memtarget_type: {
/*          if (f->memories == NULL) {
                ASSERT(f->memoryCount == dependence_count);
                f->memories = (ocrMemTarget_t **)runtimeChunkAlloc(dependence_count, sizeof(ocrMemTarget_t *));
            }
            f->memories[dependence_index] = (ocrMemTarget_t *)toinstance;
*/            break;
        }
        case allocator_type: {
            if (f->allocators == NULL) {
                ASSERT(f->allocatorCount == dependence_count);
                f->allocators = (ocrAllocator_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrAllocator_t *), NULL);
            }
            f->allocators[dependence_index] = (ocrAllocator_t *)toinstance;
            break;
        }
        case comptarget_type: {
/*            if (f->computes == NULL) {
                ASSERT(f->computeCount == dependence_count);
                f->computes = (ocrCompTarget_t **)runtimeChunkAlloc(dependence_count, sizeof(ocrCompTarget_t *));
            }
            f->computes[dependence_index] = (ocrCompTarget_t *)toinstance;
*/            break;
        }
        case workpile_type: {
/*            if (f->workpiles == NULL) {
                ASSERT(f->workpileCount == dependence_count);
                f->workpiles = (ocrWorkpile_t **)runtimeChunkAlloc(dependence_count, sizeof(ocrWorkpile_t *));
            }
            f->workpiles[dependence_index] = (ocrWorkpile_t *)toinstance;
*/            break;
        }
        case worker_type: {
            if (f->workers == NULL) {
                ASSERT(f->workerCount == dependence_count);
                f->workers = (ocrWorker_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrWorker_t *), NULL);
            }
            f->workers[dependence_index] = (ocrWorker_t *)toinstance;
            break;
        }
        case scheduler_type: {
            if (f->schedulers == NULL) {
                ASSERT(f->schedulerCount == dependence_count);
                f->schedulers = (ocrScheduler_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrScheduler_t *), NULL);
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

s32 build_deps (dictionary *dict, s32 A, s32 B, char *refstr, void ***all_instances, ocrParamList_t ***inst_params) {
    s32 i, j, k;
    s32 low, high;
    s32 l, h;

    for (i = 0; i < iniparser_getnsec(dict); i++) {
        // Go thru the secnames looking for instances of A
        if (strncasecmp(inst_str[A], iniparser_getsecname(dict, i), strlen(inst_str[A]))==0) {
            // For each secname, look up corresponding instance of A through id
            read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
            for (j = low; j <= high; j++) {
                // Parse corresponding dependences from secname
                read_range(dict, iniparser_getsecname(dict, i), refstr, &l, &h);
                // Connect A with B
                // Using a rough heuristic for now: if |from| == |to| then 1:1, else all:all TODO: What else makes sense here?
                if (h-l == high-low) {
                    k = l+j-low;
                    add_dependence(A, B, all_instances[A][j], inst_params[A][j], all_instances[B][k], inst_params[B][k], 0, 1);
                } else {
                    for (k = l; k <= h; k++) {
                        add_dependence(A, B, all_instances[A][j], inst_params[A][j], all_instances[B][k], inst_params[B][k], k-l, h-l+1);
                    }
                }
            }
        }
    }
    return 0;
}
