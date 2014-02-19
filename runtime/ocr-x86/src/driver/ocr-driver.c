/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"

#ifdef SAL_LINUX

#include "debug.h"
#include "machine-description/ocr-machine.h"
#include "ocr-config.h"
#include "ocr-db.h"
#include "ocr-edt.h"
#include "ocr-lib.h"
#include "ocr-runtime.h"
#include "ocr-types.h"
#include "ocr-sysboot.h"
#include "utils/ocr-utils.h"
#include "ocr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

u8 startMemStat = 0;

#define DEBUG_TYPE INIPARSING

const char *type_str[] = {
    "GuidType",
    "MemPlatformType",
    "MemTargetType",
    "AllocatorType",
    "CompPlatformType",
    "CommPlatformType",
    "CompTargetType",
    "WorkPileType",
    "WorkerType",
    "SchedulerType",
    "PolicyDomainType",
    "TaskType",
    "TaskTemplateType",
    "DataBlockType",
    "EventType",
};

const char *inst_str[] = {
    "GuidInst",
    "MemPlatformInst",
    "MemTargetInst",
    "AllocatorInst",
    "CompPlatformInst",
    "CommPlatformInst",
    "CompTargetInst",
    "WorkPileInst",
    "WorkerInst",
    "SchedulerInst",
    "PolicyDomainInst",
    "NULL",
    "NULL",
    "NULL",
    "NULL",
};

/* The below array defines the list of dependences */

dep_t deps[] = {
    { memtarget_type, memplatform_type, "memplatform"},
    { allocator_type, memtarget_type, "memtarget"},
    { compplatform_type, commplatform_type, "commplatform"},
    { comptarget_type, compplatform_type, "compplatform"},
    { worker_type, comptarget_type, "comptarget"},
    { scheduler_type, workpile_type, "workpile"},
    { policydomain_type, guid_type, "guid"},
    { policydomain_type, allocator_type, "allocator"},
    { policydomain_type, worker_type, "worker"},
    { policydomain_type, scheduler_type, "scheduler"},
    { policydomain_type, policydomain_type, "parent"},
    { policydomain_type, taskfactory_type, "taskfactory"},
    { policydomain_type, tasktemplatefactory_type, "tasktemplatefactory"},
    { policydomain_type, datablockfactory_type, "datablockfactory"},
    { policydomain_type, eventfactory_type, "eventfactory"},
};

extern char* populate_type(ocrParamList_t **type_param, type_enum index, dictionary *dict, char *secname);
int populate_inst(ocrParamList_t **inst_param, void **instance, int *type_counts, char ***factory_names, void ***all_factories, void ***all_instances, type_enum index, dictionary *dict, char *secname);
extern int build_deps (dictionary *dict, int A, int B, char *refstr, void ***all_instances, ocrParamList_t ***inst_params);
extern int build_deps_types (int B, void **pdinst, int pdcount, void ***all_factories, ocrParamList_t ***type_params);
extern void *create_factory (type_enum index, char *factory_name, ocrParamList_t *paramlist);
extern int read_range(dictionary *dict, char *sec, char *field, int *low, int *high);
extern void free_instance(void *instance, type_enum inst_type);

ocrGuid_t __attribute__ ((weak)) mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    // This is just to make the linker happy and shouldn't be executed
    printf("error: no mainEdt defined.\n");
    ASSERT(false);
    return NULL_GUID;
}

void **all_factories[sizeof(type_str)/sizeof(const char *)];
void **all_instances[sizeof(inst_str)/sizeof(const char *)];
int total_types = sizeof(type_str)/sizeof(const char *);
int type_counts[sizeof(type_str)/sizeof(const char *)];
int inst_counts[sizeof(inst_str)/sizeof(const char *)];
ocrParamList_t **type_params[sizeof(type_str)/sizeof(const char *)];
char **factory_names[sizeof(type_str)/sizeof(const char *)];
ocrParamList_t **inst_params[sizeof(inst_str)/sizeof(const char *)];

#ifdef ENABLE_BUILDER_ONLY

#define APP_BINARY    "CrossPlatform:app_file"
#define OUTPUT_BINARY "CrossPlatform:struct_file"
#define START_ADDRESS "CrossPlatform:start_address"
#define DRAM_OFFSET   "CrossPlatform:dram_offset"

char *app_binary;
char *output_binary;

extern int extract_functions(const char *);
extern void free_functions(void);
extern char *persistent_chunk;
extern u64 persistent_pointer;
extern u64 dram_offset;

/* Format of this file:
 *
 * +--------------------------+
 * | header size (incl) (u64) | = (5)*sizeof(u64)
 * +--------------------------+
 * |  abs. location  (u64)    | = CeMemSize - 4K
 * +--------------------------+
 * |  abs.address of PD (u64) | = abs.location + PD offset
 * +--------------------------+
 * |  size (of structs) (u64) | = persistent_pointer
 * +--------------------------+
 * |      PD->mylocation      |
 * +--------------------------+
 * |                          |
 * |     structs be here      |
 * |                          |
 * +--------------------------+
 */

void dumpStructs(void *pd, const char* output_binary, u64 start_address) {
    FILE *fp = fopen(output_binary, "w");
    u64 value, i, totu64 = 0;
    u64 offset;
    u64 *ptrs = (u64 *)&persistent_chunk;

    if(fp == NULL) printf("Unable to open file %s for writing\n", output_binary);
    else {
        
        // Write the header
        // Header size
        value = 5*sizeof(u64);
        fwrite(&value, sizeof(u64), 1, fp); totu64++;

        // Absolute location - currently read from config file
        fwrite(&start_address, sizeof(u64), 1, fp); totu64++;

        // PD address        
        offset = (u64)pd - (u64)&persistent_chunk + (u64)start_address;
        fwrite(&offset, sizeof(u64), 1, fp); totu64++;

        // Size of all structs
        fwrite(&persistent_pointer, sizeof(u64), 1, fp); totu64++;

        // myLocation
        offset = (u64)(&((ocrPolicyDomain_t *)pd)->myLocation) - (u64)&persistent_chunk + (u64)start_address;
        fwrite(&offset, sizeof(u64), 1, fp); totu64++;

        // Fix up all the pointers 
        // (FIXME: potential low-likelihood bug due to address collision; need to be improved upon)
        for(i = 0; i<(persistent_pointer/sizeof(u64)); i++) {
            if((ptrs[i] > (u64)ptrs) && (ptrs[i] < (u64)(ptrs+persistent_pointer))) {
                ptrs[i] -= (u64)ptrs; 
                ptrs[i] += start_address;
            }
        }
        fwrite(&persistent_chunk, sizeof(char), persistent_pointer, fp);
        printf("Wrote %ld bytes to %s\n", persistent_pointer+totu64*8, output_binary);
    }
    fclose(fp);
}

void builderPreamble(dictionary *dict) {

    app_binary = iniparser_getstring(dict, APP_BINARY, NULL);
    output_binary = iniparser_getstring(dict, OUTPUT_BINARY, NULL);
    dram_offset = (u64)iniparser_getlonglong(dict, DRAM_OFFSET, 0);

    if(app_binary==NULL || output_binary==NULL) {
        printf("Unable to read %s and %s; got %s and %s respectively\n", APP_BINARY, OUTPUT_BINARY, app_binary, output_binary);
    } else {
        extract_functions(app_binary);
    }
}

#endif

void bringUpRuntime(const char *inifile) {
    int i, j, count=0, nsec;
    dictionary *dict = iniparser_load(inifile);

#ifdef ENABLE_BUILDER_ONLY
    builderPreamble(dict);
#endif

    // INIT
    for (j = 0; j < total_types; j++) {
        type_params[j] = NULL; type_counts[j] = 0; factory_names[j] = NULL;
        inst_params[j] = NULL; inst_counts[j] = 0;
        all_factories[j] = NULL; all_instances[j] = NULL;
    }

    // POPULATE TYPES
    DPRINTF(DEBUG_LVL_INFO, "========= Create factories ==========\n");

    nsec = iniparser_getnsec(dict);
    for (i = 0; i < nsec; i++) {
        for (j = 0; j < total_types; j++) {
            if (strncasecmp(type_str[j], iniparser_getsecname(dict, i), strlen(type_str[j]))==0) {
                type_counts[j]++;
            }
        }
    }

    for (i = 0; i < nsec; i++) {
        for (j = 0; j < total_types; j++) {
            if (strncasecmp(type_str[j], iniparser_getsecname(dict, i), strlen(type_str[j]))==0) {
                if(type_counts[j] && type_params[j]==NULL) {
                    type_params[j] = (ocrParamList_t **)runtimeChunkAlloc(type_counts[j] * sizeof(ocrParamList_t *), (void *)1);
//                    factory_names[j] = (char **)calloc(type_counts[j], sizeof(char *));
//                    all_factories[j] = (void **)calloc(type_counts[j], sizeof(void *));
                    factory_names[j] = (char **)runtimeChunkAlloc(type_counts[j] * sizeof(char *), (void *)1);
                    all_factories[j] = (void **)runtimeChunkAlloc(type_counts[j] * sizeof(void *), (void *)1);
                    count = 0;
                }
                factory_names[j][count] = populate_type(&type_params[j][count], j, dict, iniparser_getsecname(dict, i));
                all_factories[j][count] = create_factory(j, factory_names[j][count], type_params[j][count]);
                if (all_factories[j][count] == NULL) {
                    free(factory_names[j][count]);
                    factory_names[j][count] = NULL;
                }
                count++;
            }
        }
    }

    // POPULATE INSTANCES
    DPRINTF(DEBUG_LVL_INFO, "========= Create instances ==========\n");

    for (i = 0; i < nsec; i++) {
        for (j = 0; j < total_types; j++) {
            if (strncasecmp(inst_str[j], iniparser_getsecname(dict, i), strlen(inst_str[j]))==0) {
                int low, high, count;
                count = read_range(dict, iniparser_getsecname(dict, i), "id", &low, &high);
                inst_counts[j]+=count;
            }
        }
    }

    for (i = 0; i < nsec; i++) {
        for (j = total_types-1; j >= 0; j--) {
            if (strncasecmp(inst_str[j], iniparser_getsecname(dict, i), strlen(inst_str[j]))==0) {
                if(inst_counts[j] && inst_params[j] == NULL) {
                    DPRINTF(DEBUG_LVL_INFO, "Create %d instances of %s\n", inst_counts[j], inst_str[j]);
                    inst_params[j] = (ocrParamList_t **)runtimeChunkAlloc(inst_counts[j] * sizeof(ocrParamList_t *), (void *)1);
                    all_instances[j] = (void **)runtimeChunkAlloc(inst_counts[j] * sizeof(void *), (void *)1);
//                    inst_params[j] = (ocrParamList_t **)calloc(inst_counts[j], sizeof(ocrParamList_t *));
//                    all_instances[j] = (void **)calloc(inst_counts[j], sizeof(void *));
                    count = 0;
                }
                populate_inst(inst_params[j], all_instances[j], type_counts, factory_names, all_factories, all_instances, j, dict, iniparser_getsecname(dict, i));
            }
        }
    }

    // Special case: register compPlatformFactory's functions
    ocrCompPlatformFactory_t *compPlatformFactory;
    compPlatformFactory = (ocrCompPlatformFactory_t *) all_factories[compplatform_type][0];
    if (type_counts[compplatform_type] != 1) {
        DPRINTF(DEBUG_LVL_WARN, "Only the first type of CompPlatform is used. If you don't want this behavior, please reorder!\n");
    }
    if(compPlatformFactory->setEnvFuncs != NULL) compPlatformFactory->setEnvFuncs(compPlatformFactory);

    // BUILD DEPENDENCES
    DPRINTF(DEBUG_LVL_INFO, "========= Build dependences ==========\n");

    for (i = 0; i <= 10; i++) {
        build_deps(dict, deps[i].from, deps[i].to, deps[i].refstr, all_instances, inst_params);
    }

    // Special case of policy domain pointing to types rather than instances
    for (i = 11; i <= 14; i++) {
        build_deps_types(deps[i].to, all_instances[policydomain_type],
                         inst_counts[policydomain_type], all_factories, type_params);
    }

    // START EXECUTION
    DPRINTF(DEBUG_LVL_INFO, "========= Start execution ==========\n");
    ocrPolicyDomain_t *rootPolicy;
    rootPolicy = (ocrPolicyDomain_t *) all_instances[policydomain_type][0];
   
#ifdef OCR_ENABLE_STATISTICS
    setCurrentPD(rootPolicy); // Statistics needs to know the current PD so we set it for this main thread
#endif

#ifdef ENABLE_BUILDER_ONLY
    {
        u64 start_address = iniparser_getlonglong(dict, START_ADDRESS, 0);

        for(i = 0; i < inst_counts[policydomain_type]; i++)
            dumpStructs(all_instances[policydomain_type][i], output_binary, start_address);
        free_functions();
    }
#else
    ocrPolicyDomain_t *otherPolicyDomains = NULL;
    rootPolicy->fcts.begin(rootPolicy);
    for (i = 1; i < inst_counts[policydomain_type]; i++) {
        otherPolicyDomains = (ocrPolicyDomain_t*)all_instances[policydomain_type][i];
        otherPolicyDomains->fcts.begin(otherPolicyDomains);
    }

    rootPolicy->fcts.start(rootPolicy);
    for (i = 1; i < inst_counts[policydomain_type]; i++) {
        otherPolicyDomains = (ocrPolicyDomain_t*)all_instances[policydomain_type][i];
        otherPolicyDomains->fcts.start(otherPolicyDomains);
    }
#endif
    iniparser_freedict(dict);

#ifdef ENABLE_BUILDER_ONLY
    exit(0);
#endif
}

void freeUpRuntime (void)
{
    u32 i, j;

    for (i = 0; i < total_types; i++) {
        for (j = 0; j < type_counts[i]; j++) {
            free (all_factories[i][j]);
            free (type_params[i][j]);
            free (factory_names[i][j]);
        }
        free (all_factories[i]);
    }
/*
    for (i = 0; i < total_types; i++)
        for (j = 0; j < inst_counts[i]; j++)
            free_instance(all_instances[i][j], i);
*/
    for (i = 0; i < total_types; i++) {
        for (j = 0; j < inst_counts[i]; j++) {
            if(inst_params[i][j])
                free (inst_params[i][j]);
        }
        if(inst_params[i])
            free (inst_params[i]);
        free (all_instances[i]);
    }
}

/**
 * @brief Packs user args in a contiguous chunk of memory.
 *
 * The packing contains some information at the beginning of the memory
 * that is not seen by the mainEdt and are stripped out by the runtime
 * before spawning the mainEdt.
 *
 * The encoding format is as follow:
 *      |totalLengh|argc|offsets|argv strings|
 * Size of each element:
 *      |u64|u64|u64*argc|strlen(argv[0:argc-1])+1|
 *          ^ -> '0' of the offset calculation
 * Note 
 * - totalLengh:   Total length of the packed arguments (everything minus this u64 part)
 * - argc:         Number of arguments
 * - offsets:      The offsets for each argument where the argv data is located in the chunk
 *                 Warning: Offsets are computed starting from the second u64 !
 * - argv strings: All argv strings one after the other separated by \0;
 *
 * @param argc Number of user-level arguments to pack in a DB
 * @param argv The actual arguments
 */
static void * packUserArguments(int argc, char ** argv) {
    // Prepare arguments for the mainEdt
    ASSERT(argc < 64); // For now
    u32 i;
    u64* offsets = (u64*) runtimeChunkAlloc(sizeof(u64)*argc, NULL);
    u64 argsUsed = 0ULL;
    u64 totalLength = 0;
    u32 maxArg = 0;
    // Gets all the possible offsets
    for(i = 0; i < argc; ++i) {
        // If the argument should be passed down
        offsets[maxArg++] = totalLength*sizeof(char);
        totalLength += strlen(argv[i]) + 1; // +1 for the NULL terminating char
        argsUsed |= (1ULL<<(63-i));
    }
    //--maxArg;
    // Create a memory chunk containing the parameters
    u64 extraOffset = (maxArg + 1)*sizeof(u64);
    void* ptr = (void *) runtimeChunkAlloc(totalLength + sizeof(u64) + extraOffset, NULL);

    // Copy in the values to the ptr. The format is as follows:
    // - First 4 bytes encode the size of the packed arguments 
    //   (stripped out before passing the packed args to the mainEdt)
    // - Next 4 bytes encode the number of arguments (u64) (called argc)
    // - After that, an array of argc u64 offsets is encoded.
    // - The strings are then placed after that at the offsets encoded
    //
    // The use case is therefore as follows:
    // - (The first 4 bytes are stripped before being handed over to the mainEdt)
    // - Cast the DB to a u64* and read the number of arguments and
    //   offsets (or whatever offset you need)
    // - Cast the DB to a char* and access the char* at the offset
    //   read. This will be a null terminated string.

    // Copy the metadata
    u64* dbAsU64 = (u64*)ptr;
    dbAsU64[0] = totalLength + extraOffset; // do not account for the first element
    dbAsU64[1] = (u64)maxArg;
    for(i = 2; i < maxArg+2; ++i) {
        dbAsU64[i] = offsets[i-2] + extraOffset;
    }

    // Copy the actual arguments, skipping over the first totalLength element
    char* dbAsChar = (char*) ((u64)ptr+sizeof(u64));
    while(argsUsed) {
        u32 pos = fls64(argsUsed);
        argsUsed &= ~(1ULL<<pos);
        strcpy(dbAsChar + extraOffset + offsets[63 - pos], argv[63 - pos]);
    }

    runtimeChunkFree((u64) offsets, NULL);
    return ptr;
}

int __attribute__ ((weak)) main(int argc, const char* argv[]) {
    // Parse parameters. The idea is to extract the ones relevant
    // to the runtime and pass all the other ones down to the mainEdt
    ocrConfig_t ocrConfig;
    ocrParseArgs(argc, argv, &ocrConfig);

    // Register pointer to the mainEdt
    mainEdtSet(mainEdt);

    // Pack and save user args for the mainEdt
    void * packedUserArgv = packUserArguments(ocrConfig.userArgc, ocrConfig.userArgv);
    userArgsSet(packedUserArgv);

    // Set up the runtime
    ocrInit(&ocrConfig);

    // Here the runtime is fully functional and
    // the "blessed" worker will execute the mainEdt

    startMemStat = 1;
    ocrFinalize();

    return 0;
}

#endif
