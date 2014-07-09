/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef SAL_LINUX

#include "allocator/allocator-all.h"
#include "comm-api/comm-api-all.h"
#include "comm-platform/comm-platform-all.h"
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
#include "task/task-all.h"
#include "worker/worker-all.h"
#include "workpile/workpile-all.h"

#include "ocr-sysboot.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>

#define MAX_INSTANCES 64
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

#define INI_GET_LONG(KEY, VAR, DEF) \
    VAR = (s64) iniparser_getint(dict, KEY, DEF); \
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

/* Read a mix of ranges & CSV */
s32 read_values(dictionary *dict, char *sec, char *field, s32 *values_array) {
    char key[MAX_KEY_SZ];
    s32 low, high;
    u32 count = 0;
    char *values;
    u32 i;

    snprintf(key, MAX_KEY_SZ, "%s:%s", sec, field);
    values = iniparser_getstring(dict, key, NULL);
    do {
        if(strstr(values, "-")) {
            sscanf(values, "%d-%d", &low, &high);
            for(i=count; i<=count+high-low; i++) values_array[i] = low+i-count;
            count += high-low+1;
        } else {  // Assume integer
            sscanf(values, "%d", &low);
            values_array[count] = low;
            count++;
        }
        while((*(char *)values)!='\0' && (*(char *)values)!=',') values++;
        if(*(char *)values == ',') values++;
    } while((*(char *)values)!='\0');
    return count;
}

// TODO: expand to parse comma separated values & ranges iterating the below thru strtok with ,
// TODO: eventually, extend this to expressions
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
    *low = lo;
    *high = hi;
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
    case commapi_type:
        ALLOC_PARAM_LIST(*type_param, paramListCommApiFact_t);
        break;
    case compplatform_type: {
        compPlatformType_t mytype = -1;
        TO_ENUM (mytype, typestr, compPlatformType_t, compplatform_types, compPlatformMax_id);
        switch (mytype) {
#ifdef ENABLE_COMP_PLATFORM_PTHREAD
        case compPlatformPthread_id: {
            int value = 0;
            ALLOC_PARAM_LIST(*type_param, paramListCompPlatformPthread_t);
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "stacksize");
            INI_GET_INT (key, value, -1);
            ((paramListCompPlatformPthread_t *)(*type_param))->stackSize = (value==-1)?0:value;
        }
        break;
#endif
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
    case taskfactory_type:
        ALLOC_PARAM_LIST(*type_param, paramListTaskFact_t);
        break;
    case tasktemplatefactory_type:
        ALLOC_PARAM_LIST(*type_param, paramListTaskTemplateFact_t);
        break;
    case datablockfactory_type:
        ALLOC_PARAM_LIST(*type_param, paramListDataBlockFact_t);
        break;
    case eventfactory_type:
        ALLOC_PARAM_LIST(*type_param, paramListEventFact_t);
        break;
    case commplatform_type:
        ALLOC_PARAM_LIST(*type_param, paramListCommPlatformFact_t);
        break;
    default:
        DPRINTF(DEBUG_LVL_WARN, "Error: %d index unexpected\n", index);
        break;
    }

    return strdup(typestr);
}

ocrCommApiFactory_t *create_factory_commapi (char *name, ocrParamList_t *paramlist) {
    commApiType_t mytype = commApiMax_id;
    TO_ENUM (mytype, name, commApiType_t, commapi_types, commApiMax_id);

    if (mytype == commApiMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a commapi factory of type %d\n", mytype);
        return newCommApiFactory(mytype, paramlist);
    }
}

ocrCommPlatformFactory_t *create_factory_commplatform (char *name, ocrParamList_t *paramlist) {
    commPlatformType_t mytype = commPlatformMax_id;
    TO_ENUM (mytype, name, commPlatformType_t, commplatform_types, commPlatformMax_id);

    if (mytype == commPlatformMax_id) {
        DPRINTF(DEBUG_LVL_WARN, "Unrecognized type %s\n", name);
        return NULL;
    } else {
        DPRINTF(DEBUG_LVL_INFO, "Creating a commplatform factory of type %d\n", mytype);
        return newCommPlatformFactory(mytype, paramlist);
    }
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
    case commapi_type:
        new_factory = (void *)create_factory_commapi(factory_name, paramlist);
        break;
    case commplatform_type:
        new_factory = (void *)create_factory_commplatform(factory_name, paramlist);
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
    case taskfactory_type:
        new_factory = (void *)create_factory_task(factory_name, paramlist);
        break;
    case tasktemplatefactory_type:
        new_factory = (void *)create_factory_tasktemplate(factory_name, paramlist);
        break;
    case datablockfactory_type:
        new_factory = (void *)create_factory_datablock(factory_name, paramlist);
        break;
    case eventfactory_type:
        new_factory = (void *)create_factory_event(factory_name, paramlist);
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
    s32 value = 0;

    read_range(dict, secname, "id", &low, &high);

    snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "type");
    INI_GET_STR (key, inststr, "");

    for (i = 0; i < type_counts[index]; i++) {
        if (factory_names[index][i] && (0 == strncmp(factory_names[index][i], inststr, 1+strlen(factory_names[index][i])))) break;
    }
    if (factory_names[index][i] == NULL || strncmp(factory_names[index][i], inststr, 1+strlen(factory_names[index][i]))) {
        DPRINTF(DEBUG_LVL_WARN, "Unknown type %s while reading key %s\n", inststr, key);
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
            memPlatformType_t mytype = -1;

            TO_ENUM (mytype, inststr, memPlatformType_t, memplatform_types, memPlatformMax_id);
            switch (mytype) {
#ifdef ENABLE_MEM_PLATFORM_FSIM
            case memPlatformFsim_id: {
                extern u64 end_marker;
                ALLOC_PARAM_LIST(inst_param[j], paramListMemPlatformFsim_t);
                ((paramListMemPlatformFsim_t *)inst_param[j])->start = end_marker;
                break;
            }
#endif
            default:
                ALLOC_PARAM_LIST(inst_param[j], paramListMemPlatformInst_t);
                break;
            }

#ifdef ENABLE_MEM_PLATFORM_FSIM
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "start");
            ((paramListMemPlatformFsim_t*)inst_param[j])->start += (u64)iniparser_getlonglong(dict, key, 0);
            // REC: I don't think we need to revise the size as it was done because that implies that
            // all memories are L1s
            // This needs to be revised anyways because whatever we do, it is brittle
#endif
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "size");
            ((paramListMemPlatformInst_t *)inst_param[j])->size = (u64)iniparser_getlonglong(dict, key, 0);

            instance[j] = (void *)((ocrMemPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created memplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case memtarget_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListMemTargetInst_t);
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "size");
            ((paramListMemTargetInst_t *)inst_param[j])->size = (u64)iniparser_getlonglong(dict, key, 0);
            ((paramListMemTargetInst_t *)inst_param[j])->level = 0;
            if (key_exists(dict, secname, "level")) {
                snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "level");
                INI_GET_INT (key, value, -1);
                ((paramListMemTargetInst_t *)inst_param[j])->level = (value==-1)?0:value;
            }
            instance[j] = (void *)((ocrMemTargetFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created memtarget of type %s, index %d\n", inststr, j);
        }
        break;
    case allocator_type:
        for (j = low; j<=high; j++) {
            allocatorType_t mytype = -1;
            TO_ENUM (mytype, inststr, allocatorType_t, allocator_types, allocatorMax_id);
            switch (mytype) {
#ifdef ENABLE_ALLOCATOR_TLSF
                case allocatorTlsf_id: {
                    ALLOC_PARAM_LIST(inst_param[j], paramListAllocatorTlsf_t);
                    ((paramListAllocatorTlsf_t *)inst_param[j])->sliceCount = 0;
                    ((paramListAllocatorTlsf_t *)inst_param[j])->sliceSize  = 0;
                    if (key_exists(dict, secname, "slicesize") || key_exists(dict, secname, "slicecount")) {
                        if (key_exists(dict, secname, "slicesize") && key_exists(dict, secname, "slicecount")) {
                            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "slicecount");
                            INI_GET_INT (key, value, -1);
                            ((paramListAllocatorTlsf_t *)inst_param[j])->sliceCount = (value==-1)?0:value;
                            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "slicesize");
                            INI_GET_INT (key, value, -1);
                            ((paramListAllocatorTlsf_t *)inst_param[j])->sliceSize = (value==-1)?0:value;
                            if (((paramListAllocatorTlsf_t *)inst_param[j])->sliceCount != 0 &&
                                ((paramListAllocatorTlsf_t *)inst_param[j])->sliceSize < 256) {
                                DPRINTF(DEBUG_LVL_INFO, "When slicing allocators, minimum slice size is 256 bytes.  Suppressing slicing.\n");
                                ((paramListAllocatorTlsf_t *)inst_param[j])->sliceCount = 0;
                            }
                        } else {
                            DPRINTF(DEBUG_LVL_INFO, "slicesize and slicecount must be given together or not at all.\n");
                        }
                    }
                }
                break;
#endif
                default:
                    ALLOC_PARAM_LIST(inst_param[j], paramListAllocatorInst_t);
                break;
            }
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "size");
            ((paramListAllocatorInst_t *)inst_param[j])->size = (u64)iniparser_getlonglong(dict, key, 0);
            instance[j] = (void *)((ocrAllocatorFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created allocator of type %s, index %d\n", inststr, j);
        }
        break;


    case commapi_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListCommApiInst_t);
            instance[j] = (void *)((ocrCommPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created commapi of type %s, index %d\n", inststr, j);
        }
        break;
    case commplatform_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListCommPlatformInst_t);
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "location");
            //INI_GET_LONG (key, value, -1); TODO: Decide if this is needed
            //value |= j;  // Or the id into value for location.
            //((paramListCommPlatformInst_t *)inst_param[j])->location = (ocrLocation_t)value;
            instance[j] = (void *)((ocrCommPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created commplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case compplatform_type:
        for (j = low; j<=high; j++) {

            compPlatformType_t mytype = -1;

            TO_ENUM (mytype, inststr, compPlatformType_t, compplatform_types, compPlatformMax_id);
            switch (mytype) {
#ifdef ENABLE_COMP_PLATFORM_PTHREAD
            case compPlatformPthread_id: {
                ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformPthread_t);

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
#endif
            default:
                ALLOC_PARAM_LIST(inst_param[j], paramListCompPlatformInst_t);
                break;
            }

            instance[j] = (void *)((ocrCompPlatformFactory_t *)factory)->instantiate(factory, inst_param[j]);
            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created compplatform of type %s, index %d\n", inststr, j);
        }
        break;
    case comptarget_type:
        for (j = low; j<=high; j++) {
            ALLOC_PARAM_LIST(inst_param[j], paramListCompTargetInst_t);
            instance[j] = (void *)((ocrCompTargetFactory_t *)factory)->instantiate(factory, inst_param[j]);
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
#ifdef ENABLE_WORKER_HC
            case workerHc_id: {
                char *workerstr;
                char workertypekey[MAX_KEY_SZ];
                ocrWorkerType_t workertype = MAX_WORKERTYPE;

                snprintf(workertypekey, MAX_KEY_SZ, "%s:%s", secname, "workertype");
                INI_GET_STR (workertypekey, workerstr, "");
                TO_ENUM (workertype, workerstr, ocrWorkerType_t, ocrWorkerType_types, MAX_WORKERTYPE-1);
                workertype += 1;  // because workertype is 1-indexed, not 0-indexed
                if (workertype == MAX_WORKERTYPE) workertype = SLAVE_WORKERTYPE; // TODO: is this a reasonable default?
                ALLOC_PARAM_LIST(inst_param[j], paramListWorkerHcInst_t);
                ((paramListWorkerHcInst_t *)inst_param[j])->workerType = workertype;
                ((paramListWorkerHcInst_t *)inst_param[j])->workerId = j; // using "id" for now; TODO: decide if a separate key is needed
            }
            break;
#endif
#ifdef ENABLE_WORKER_CE
            case workerCe_id: {
                char *workerstr;
                char workertypekey[MAX_KEY_SZ];
                ocrWorkerType_t workertype = MAX_WORKERTYPE;

                snprintf(workertypekey, MAX_KEY_SZ, "%s:%s", secname, "workertype");
                INI_GET_STR (workertypekey, workerstr, "");
                TO_ENUM (workertype, workerstr, ocrWorkerType_t, ocrWorkerType_types, MAX_WORKERTYPE-1);
                workertype += 1;  // because workertype is 1-indexed, not 0-indexed
                if (workertype == MAX_WORKERTYPE) workertype = SLAVE_WORKERTYPE; // TODO: is this a reasonable default?
                ALLOC_PARAM_LIST(inst_param[j], paramListWorkerCeInst_t);
                ((paramListWorkerCeInst_t *)inst_param[j])->workerType = workertype;
                ((paramListWorkerCeInst_t *)inst_param[j])->workerId = j; // using "id" for now; TODO: decide if a separate key is needed
            }
            break;
#endif
#ifdef ENABLE_WORKER_XE
            case workerXe_id: {
                char *workerstr;
                char workertypekey[MAX_KEY_SZ];
                ocrWorkerType_t workertype = MAX_WORKERTYPE;

                snprintf(workertypekey, MAX_KEY_SZ, "%s:%s", secname, "workertype");
                INI_GET_STR (workertypekey, workerstr, "");
                TO_ENUM (workertype, workerstr, ocrWorkerType_t, ocrWorkerType_types, MAX_WORKERTYPE-1);
                workertype += 1;  // because workertype is 1-indexed, not 0-indexed
                if (workertype == MAX_WORKERTYPE) workertype = SLAVE_WORKERTYPE; // TODO: is this a reasonable default?
                ALLOC_PARAM_LIST(inst_param[j], paramListWorkerXeInst_t);
                // ((paramListWorkerXeInst_t *)inst_param[j])->workerType = workertype;  TODO: This field doesn't exist for XE's.  Should it?  BRN
                ((paramListWorkerXeInst_t *)inst_param[j])->workerId = j; // using "id" for now; TODO: decide if a separate key is needed
            }
            break;
#endif
            default:
                ASSERT (0); // Unimplemented worker type.
                //ALLOC_PARAM_LIST(inst_param[j], paramListWorkerInst_t);
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
#ifdef ENABLE_SCHEDULER_HC
            case schedulerHc_id: {
                ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerHcInst_t);
                snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "workeridfirst");
                INI_GET_INT (key, value, -1);
                ((paramListSchedulerHcInst_t *)inst_param[j])->workerIdFirst = value;
            }
            break;
#endif
#ifdef ENABLE_SCHEDULER_CE
            case schedulerCe_id: {
                ALLOC_PARAM_LIST(inst_param[j], paramListSchedulerCeInst_t);
            }
            break;
#endif
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
            policyDomainType_t mytype = policyDomainMax_id;
#ifdef OCR_ENABLE_STATISTICS
            // HUGE HUGE HUGE HUGE HUGE HACK. This needs to be parsed
            // but for now just passing some default one
            ocrStatsFactory_t *sf = newStatsFactory(statisticsDefault_id, NULL);
#endif

            TO_ENUM (mytype, inststr, policyDomainType_t, policyDomain_types, policyDomainMax_id);
            switch (mytype) {
#ifdef ENABLE_POLICY_DOMAIN_HC
            case policyDomainHc_id: {
                ALLOC_PARAM_LIST(inst_param[j], paramListPolicyDomainHcInst_t);
                if (key_exists(dict, secname, "rank")) {
                    value = get_key_value(dict, secname, "rank", j-low);
                    ((paramListPolicyDomainHcInst_t *)inst_param[j])->rank = (u32)value;
                } else {
                    ((paramListPolicyDomainHcInst_t *)inst_param[j])->rank = (u32)-1;
                }
            }
            break;
#endif
#ifdef ENABLE_POLICY_DOMAIN_CE
            case policyDomainCe_id: {
                ALLOC_PARAM_LIST(inst_param[j], paramListPolicyDomainCeInst_t);
                if (key_exists(dict, secname, "xecount")) {
                    value = get_key_value(dict, secname, "xecount", j-low);
                    ((paramListPolicyDomainCeInst_t *)inst_param[j])->xeCount = (u32)value;
                } else {
                    ((paramListPolicyDomainCeInst_t *)inst_param[j])->xeCount = (u32)8;
                }
                if (key_exists(dict, secname, "neighborcount")) {
                    value = get_key_value(dict, secname, "neighborcount", j-low);
                    ((paramListPolicyDomainCeInst_t *)inst_param[j])->neighborCount = (u32)value;
                } else {
                    ((paramListPolicyDomainCeInst_t *)inst_param[j])->neighborCount = (u32)0;
                }
            }
            break;
#endif
            default:
                ALLOC_PARAM_LIST(inst_param[j], paramListPolicyDomainInst_t);
                break;
            }

            // TODO: Redo the below to mirror rank config changes
            snprintf(key, MAX_KEY_SZ, "%s:%s", secname, "location");
            if(INI_IS_RANGE(key)) {
                ((paramListPolicyDomainInst_t*)inst_param[j])->location =  j-1;  // FIXME: HACKHACKHACK
            } else {
                INI_GET_INT (key, value, -1);
                ((paramListPolicyDomainInst_t*)inst_param[j])->location = value;
            }

            instance[j] = (void *)((ocrPolicyDomainFactory_t *)factory)->instantiate(
                              factory, NULL, inst_param[j]);

            if (instance[j])
                DPRINTF(DEBUG_LVL_INFO, "Created policy domain of index %d\n", j);

        }
        break;
    default:
        DPRINTF(DEBUG_LVL_WARN, "Error: %d index unexpected\n", index);
        break;
    }
    return 0;
}

void add_dependence (type_enum fromtype, type_enum totype,
                     void *frominstance, ocrParamList_t *fromparam,
                     void *toinstance, ocrParamList_t *toparam,
                     s32 dependence_index, s32 dependence_count) {
    switch(fromtype) {
    case guid_type:
    case memplatform_type:
    case commplatform_type:
    case compplatform_type:
    case workpile_type:
        DPRINTF(DEBUG_LVL_WARN, "Unexpected: this type should have no dependences! (incorrect dependence: %d to %d)\n", fromtype, totype);
        break;

    case commapi_type: {
        ocrCommApi_t *f = (ocrCommApi_t *)frominstance;
        DPRINTF(DEBUG_LVL_INFO, "CommApi %d to %d\n", fromtype, totype);
        f->commPlatform = (ocrCommPlatform_t *)toinstance;
        break;
    }
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
        default:
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
            if (f->guidProviders == NULL) {
                f->guidProviderCount = dependence_count;
                f->guidProviders = (ocrGuidProvider_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrGuidProvider_t *), NULL);
            }
            f->guidProviders[dependence_index] = (ocrGuidProvider_t *)toinstance;
            break;
        }
        case allocator_type: {
            if (f->allocators == NULL) {
                f->allocatorCount = dependence_count;
                f->allocators = (ocrAllocator_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrAllocator_t *), NULL);
            }
            f->allocators[dependence_index] = (ocrAllocator_t *)toinstance;
            break;
        }
        case commapi_type: {
            if (f->commApis == NULL) {
                f->commApiCount = dependence_count;
                f->commApis = (ocrCommApi_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrCommApi_t *), NULL);
            }
            f->commApis[dependence_index] = (ocrCommApi_t *)toinstance;
            break;
        }
        case worker_type: {
            if (f->workers == NULL) {
                f->workerCount = dependence_count;
                f->workers = (ocrWorker_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrWorker_t *), NULL);
            }
            f->workers[dependence_index] = (ocrWorker_t *)toinstance;
            break;
        }
        case scheduler_type: {
            if (f->schedulers == NULL) {
                f->schedulerCount = dependence_count;
                f->schedulers = (ocrScheduler_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrScheduler_t *), NULL);
            }
            f->schedulers[dependence_index] = (ocrScheduler_t *)toinstance;
            break;
        }
        case taskfactory_type: {
            if (f->taskFactories == NULL) {
                f->taskFactoryCount = dependence_count;
                f->taskFactories = (ocrTaskFactory_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrTaskFactory_t *), NULL);
            }
            f->taskFactories[dependence_index] = (ocrTaskFactory_t *)toinstance;
            break;
        }
        case tasktemplatefactory_type: {
            if (f->taskTemplateFactories == NULL) {
                f->taskTemplateFactoryCount = dependence_count;
                f->taskTemplateFactories = (ocrTaskTemplateFactory_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrTaskTemplateFactory_t *), NULL);
            }
            f->taskTemplateFactories[dependence_index] = (ocrTaskTemplateFactory_t *)toinstance;
            break;
        }
        case datablockfactory_type: {
            if (f->dbFactories == NULL) {
                f->dbFactoryCount = dependence_count;
                f->dbFactories = (ocrDataBlockFactory_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrDataBlockFactory_t *), NULL);
            }
            f->dbFactories[dependence_index] = (ocrDataBlockFactory_t *)toinstance;
            break;
        }
        case eventfactory_type: {
            if (f->eventFactories == NULL) {
                f->eventFactoryCount = dependence_count;
                f->eventFactories = (ocrEventFactory_t **)runtimeChunkAlloc(dependence_count * sizeof(ocrEventFactory_t *), NULL);
            }
            f->eventFactories[dependence_index] = (ocrEventFactory_t *)toinstance;
            break;
        }
        case policydomain_type: {
            ocrPolicyDomain_t* t = (ocrPolicyDomain_t*)toinstance;
            f->parentLocation = (u64) t->myLocation;
            f->parentPD = t; // FIXME: PD2Location
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
    s32 l;
    s32 depcount;
    s32 values_array[MAX_INSTANCES];

    for (i = 0; i < iniparser_getnsec(dict); i++) {
        // Go thru the secnames looking for instances of A
        if (strncasecmp(inst_str[A], iniparser_getsecname(dict, i), strlen(inst_str[A]))==0) {
            // For each secname, look up corresponding instance of A through id
            read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
            for (j = low; j <= high; j++) {
                // Parse corresponding dependences from secname
                depcount = read_values(dict, iniparser_getsecname(dict, i), refstr, values_array);
                // Connect A with B
                // Using a rough heuristic for now: if |from| == |to| then 1:1, else all:all TODO: What else makes sense here?
                if (depcount == high-low+1) {
                    k = values_array[j-low];
                    add_dependence(A, B, all_instances[A][j], inst_params[A][j], all_instances[B][k], inst_params[B][k], 0, 1);
                } else {
                    for (l = 0; l < depcount; l++) {
                        k = values_array[l];
                        add_dependence(A, B, all_instances[A][j], inst_params[A][j], all_instances[B][k], inst_params[B][k], l, depcount);
                    }
                }
            }
        }
    }
    return 0;
}

s32 build_deps_types (s32 B, void **pdinst, int pdcount, void ***all_factories, ocrParamList_t ***type_params) {
    s32 i;

    for (i = 0; i < pdcount; i++) {
        add_dependence(policydomain_type, B, pdinst[i], NULL, all_factories[B][0], NULL, 0, 1);
    }

    // FIXME: The above is highly simplified, needs review/change

    return 0;
}

#endif
