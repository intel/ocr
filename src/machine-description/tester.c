#include <stdio.h>
#include <string.h>
#include <iniparser.h>

#include <ocr-allocator.h>
#include <ocr-comp-platform.h>
#include <ocr-mem-platform.h>
#include <ocr-mem-target.h>
#include <ocr-datablock.h>
#include <ocr-event.h>

#define INI_GET_INT(KEY, VAR, DEF) VAR = (int) iniparser_getint(dict, KEY, DEF); if (VAR==DEF){ printf("Key %s not found or invalid!\n", KEY); }
#define INI_GET_STR(KEY, VAR, DEF) VAR = (char *) iniparser_getstring(dict, KEY, DEF); if (!strcmp(VAR, DEF)){ printf("Key %s not found or invalid!\n", KEY); }
#define TO_ENUM(VAR, STR, TYPE, TYPESTR, COUNT) {int kount; for (kount=0; kount<COUNT; kount++) {if (!strcmp(STR, TYPESTR[kount])) VAR=(TYPE)kount;} }
#define INI_IS_RANGE(KEY) strstr((char *) iniparser_getstring(dict, KEY, ""), "-")
#define INI_GET_RANGE(KEY, LOW, HIGH) sscanf(iniparser_getstring(dict, KEY, ""), "%d-%d", &LOW, &HIGH)

//TODO: expand all the below to include all sections
typedef enum {
    allocator_type,
    memplatform_type,
    memtarget_type,
} type_enum;

const char *type_str[] = {
    "AllocatorType",
    "MemPlatformType",
    "MemTargetType",
};

int type_counts[sizeof(type_str)/sizeof(const char *)];
ocrParamList_t **type_params[sizeof(type_str)/sizeof(const char *)]; 

typedef enum {
    allocator_inst,
    memplatform_inst,
    memtarget_inst,
} inst_enum;

const char *inst_str[] = {
    "AllocatorInst",
    "MemPlatformInst",
    "MemTargetInst",
};

int inst_counts[sizeof(inst_str)/sizeof(const char *)];
ocrParamList_t **inst_params[sizeof(inst_str)/sizeof(const char *)]; 

typedef struct {
    int from;
    int to;
    char *refstr;
} dep_t;

/* The above struct defines dependency "from" -> "to" using "refstr" as reference */

dep_t deps[] = {
    { 0, 2, "memtarget"},
    { 2, 1, "memplatform"},
};

// TODO: expand to parse comma separated values & ranges iterating the below thru strtok with ,
// TODO: stretch goal, extend this to expressions
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

int populate_type(ocrParamList_t **type_param, int index, int type_index, int type_count, dictionary *dict, char *secname)
{
    int i;
    char *typestr;
    char key[64];

    // TODO: populate type-specific fields
    printf("Populating %s count = %d; %d/%d\n", secname, index, type_index, type_count);
    switch (index) {
    case 0:
        ALLOC_PARAM_LIST(type_param[type_index], paramListAllocatorFact_t);
        break;
    case 1:
        ALLOC_PARAM_LIST(type_param[type_index], paramListMemPlatformFact_t);
        break;
    case 2:
        ALLOC_PARAM_LIST(type_param[type_index], paramListMemTargetFact_t);
        break;
    default:
        printf("Error: %d index unexpected\n", type_index);
        break;
    }

    snprintf(key, 64, "%s:%s", secname, "name");
    INI_GET_STR (key, typestr, "");
    type_param[type_index]->misc = strdup(typestr);

    return 0; 
}

int populate_inst(ocrParamList_t **inst_param, int index, int inst_index, int inst_count, dictionary *dict, char *secname)
{
    int i, low, high, count, j;
    char *inststr;
    char key[64];

    printf("Populating %s count = %d; %d/%d\n", secname, index, inst_index, inst_count);

    count = read_range(dict, secname, "id", &low, &high); 

    //TODO: case specific initialization
    switch (index) {
    case 0:
        for (j = low; j<=high; j++) { 
            paramListAllocatorInst_t *t;
            ALLOC_PARAM_LIST(inst_param[j], paramListAllocatorInst_t);
            t = (paramListAllocatorInst_t *)inst_param[j];
            snprintf(key, 64, "%s:%s", secname, "size");
            INI_GET_INT(key, t->size, -1);
            t->memtarget_count = 0;
        }
        break;
    case 1:
        for (j = low; j<=high; j++) 
            ALLOC_PARAM_LIST(inst_param[j], paramListMemPlatformInst_t);
        break;
    case 2:
        for (j = low; j<=high; j++) {
            paramListMemTargetInst_t *t;
            ALLOC_PARAM_LIST(inst_param[j], paramListMemTargetInst_t);
            t = (paramListMemTargetInst_t *)inst_param[j];
            t->memplatform_count = 0;
            snprintf(key, 64, "%s:%s", secname, "mem_start");
            INI_GET_INT(key, t->start, -1);
            snprintf(key, 64, "%s:%s", secname, "mem_size");
            INI_GET_INT(key, t->size, -1);
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
            if (0 == strcmp(inst_param[j]->misc, type_params[index][i]->misc)) found = 1;
        }
        if(found==0) printf("Unknown type (%s) encountered!\n", inst_param[j]->misc);
    }

    return 0;
}

void add_dependence (int fromtype, ocrParamList_t *from, ocrParamList_t *to, int dependence_index, int dependence_count)
{
    
    switch(fromtype) {
    case 0: {
            paramListAllocatorInst_t *t = (paramListAllocatorInst_t *)from;
            printf("Allocator %s to %s\n", from->misc, to->misc);
            if (t->memtarget_count == 0) {
                t->memtarget_count = dependence_count;
                t->memtargets = (paramListMemTargetInst_t *)malloc(sizeof(paramListMemTargetInst_t *)*dependence_count);
            }
            t->memtargets[dependence_index] = (paramListMemTargetInst_t *)to;
            break;
        }
    case 1:
        printf("Memplatform %s to %s\n", from->misc, to->misc);
        break;
    case 2: {
            paramListMemTargetInst_t *t = (paramListMemTargetInst_t *)from;
            printf("Memtarget %s to %s\n", from->misc, to->misc);
            if (t->memplatform_count == 0) {
                t->memplatform_count = dependence_count;
                t->memplatforms = (paramListMemPlatformInst_t *)malloc(sizeof(paramListMemPlatformInst_t *)*dependence_count);
            }
            t->memplatforms[dependence_index] = (paramListMemPlatformInst_t *)to;
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
                    // TODO: populate the param that points to the ref
                    add_dependence(A, inst_params[A][j], inst_params[B][k], k-l, h-l+1);
                }
            }
        }
    }
    return 0;
}

 
/* Utility functions to dump structs (for debug) */

void dump_allocator_fact(paramListAllocatorFact_t *allocFact)
{
    if (allocFact == NULL) return;
    printf("AllocFact: %s\n", allocFact->base.misc);
}

void dump_allocator_inst(paramListAllocatorInst_t *allocInst)
{
    int j;
    if (allocInst == NULL) return;
    printf("AllocInst: %s; %d; %d\t", allocInst->base.misc, allocInst->size, allocInst->memtarget_count);
    printf("\n");
}

void dump_memtarget_fact(paramListMemTargetFact_t *mtargetFact)
{
    if (mtargetFact == NULL) return;
    printf ("MemtargetFact: %s\n", mtargetFact->base.misc);
}

void dump_memtarget_inst(paramListMemTargetInst_t *mtargetInst)
{
    if (mtargetInst == NULL) return;
    printf ("MemtargetInst: %s (%x:%x); %d\n", mtargetInst->base.misc, mtargetInst->start, mtargetInst->size, mtargetInst->memplatform_count);
}

void dump_memplatform_fact(paramListMemPlatformFact_t *mplatformFact)
{
    if (mplatformFact == NULL) return;
    printf ("MemplatformFact: %s\n", mplatformFact->base.misc);
}

void dump_memplatform_inst(paramListMemPlatformInst_t *mplatformInst)
{
    if (mplatformInst == NULL) return;
    printf ("MemplatformInst: %s\n", mplatformInst->base.misc);
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

//    printf("Sec %d\n", iniparser_getnsec(dict));
//    for (i=0; i< iniparser_getnsec(dict); i++) printf("%s\n", iniparser_getsecname(dict, i));
//    printf("Sec has %d entries\n", iniparser_getsecnkeys(dict, "allocator0"));

    total_types = sizeof(type_str)/sizeof(const char *);

    // INIT
    for (j = 0; j < total_types; j++) {
        type_params[j] = NULL; type_counts[j] = 0;
        inst_params[j] = NULL; inst_counts[j] = 0;
    }

    // POPULATE TYPES
    for (i = 0; i < iniparser_getnsec(dict); i++)
        for (j = 0; j < total_types; j++)
            if (strncasecmp(type_str[j], iniparser_getsecname(dict, i), strlen(type_str[j]))==0) type_counts[j]++;
        
    for (i = 0; i < iniparser_getnsec(dict); i++) {
        for (j = 0; j < total_types; j++) {
            if (strncasecmp(type_str[j], iniparser_getsecname(dict, i), strlen(type_str[j]))==0) { 
                if(type_counts[j] && type_params[j]==NULL) { 
                    printf("Alloc %d types of %s\n", type_counts[j], type_str[j]); 
                    type_params[j] = (ocrParamList_t **)malloc(type_counts[j] * sizeof(ocrParamList_t *));
                    count = 0;
                }
                populate_type(type_params[j], j, count++, type_counts[j], dict, iniparser_getsecname(dict, i));
            }
        }
    }

    printf("\n\n");

    // POPULATE INSTANCES
    for (i = 0; i < iniparser_getnsec(dict); i++)
        for (j = 0; j < total_types; j++)
            if (strncasecmp(inst_str[j], iniparser_getsecname(dict, i), strlen(inst_str[j]))==0) { 
                int low, high, count;
                count = read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
                inst_counts[j]+=count;
            }
          
    for (i = 0; i < iniparser_getnsec(dict); i++) {
        for (j = 0; j < total_types; j++) {
            if (strncasecmp(inst_str[j], iniparser_getsecname(dict, i), strlen(inst_str[j]))==0) { 
                if(inst_counts[j] && inst_params[j] == NULL) { 
                    printf("Alloc %d instances of %s\n", inst_counts[j], inst_str[j]); 
                    inst_params[j] = (ocrParamList_t **)malloc(inst_counts[j] * sizeof(ocrParamList_t *));
                    count = 0;
                }
                populate_inst(inst_params[j], j, count++, inst_counts[j], dict, iniparser_getsecname(dict, i));
            }
        }
    }

    printf("\n\n");

    // BUILD DEPENDENCES
    for (i = 0; i < sizeof(deps)/sizeof(dep_t); i++) {
        build_deps(dict, deps[i].from, deps[i].to, deps[i].refstr);
    }

    printf("---------------------------\n");
    for(i = 0; i<type_counts[0]; i++) { dump_allocator_fact ((paramListAllocatorFact_t *)type_params[0][i]); }
    printf("---------------------------\n");

    printf("---------------------------\n");
    for(i = 0; i<type_counts[2]; i++) { dump_memtarget_fact ((paramListMemTargetFact_t *)type_params[2][i]); }
    printf("---------------------------\n");

    printf("---------------------------\n");
    for(i = 0; i<inst_counts[0]; i++) { dump_allocator_inst ((paramListAllocatorInst_t *)inst_params[0][i]); }
    printf("---------------------------\n");

    printf("---------------------------\n");
    for(i = 0; i<inst_counts[2]; i++) { printf("%d, ", i); dump_memtarget_inst ((paramListMemTargetInst_t *)inst_params[2][i]); }
    printf("---------------------------\n");

    return 0;
}
