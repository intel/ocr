/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_OCR_LIB
#include "debug.h"
#include "machine-description/ocr-machine.h"
#include "extensions/ocr-lib.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define __USE_GNU
#include <pthread.h>

enum {
    OPT_NONE, OPT_CONFIG, OPT_VERSION, OPT_HELP
};

// Helper methods
static struct options {
    char *flag;
    char *env_flag;
    s32 option;
    char *help;
} ocrOptionDesc[] = {
    {
        "cfg", "OCR_CONFIG", OPT_CONFIG, "-ocr:cfg <file> : the OCR runtime configuration file to use."
    },
    {
        "version", "", OPT_VERSION, "-ocr:version : print OCR version"
    },
    {
        "help", "", OPT_HELP, "-ocr:help : print this message"
    },
    {
        NULL, NULL, 0, NULL
    }
};

static void printHelp(void) {
    struct options *p;

    fprintf(stderr, "Usage: program [<OCR options>] [<program options>]\n");
    fprintf(stderr, "OCR options:\n");

    for (p = ocrOptionDesc; p->flag; ++p)
        if (p->help)
            fprintf(stderr, "    %s, env: %s\n", p->help, p->env_flag);

    fprintf(stderr, "\n");
    fprintf(stderr, "https://github.com/01org/ocr\n");
}

static void printVersion(void) {
    fprintf(stderr, "Open Community Runtime (OCR) %s%s\n", "0.8", "");
}

static void setIniFile(ocrConfig_t * ocrConfig, const char * value) {
    struct stat st;
    if (stat(value, &st) != 0) {
        fprintf(stderr, "error: cannot find runtime configuration file: %s\n", value);
        exit(1);
    }
    ocrConfig->iniFile = value;
}

static inline void checkNextArgExists(s32 i, s32 argc, char * option) {
    if (i == argc) {
        printf("No argument for OCR option %s\n", option);
        ASSERT(false);
    }
}

static void checkOcrOption(ocrConfig_t * ocrConfig) {
    if (ocrConfig->iniFile == NULL) {
        fprintf(stderr, "error: no runtime configuration file provided\n");
        exit(1);
    }
}

static void readFromEnv(ocrConfig_t * ocrConfig) {
    // Go over OCR options description and check
    // if some of the env variables are set.
    struct options  *p;
    for (p = ocrOptionDesc; p->flag; ++p) {
        char * opt = getenv(p->env_flag);
        // If variable defined and has value
        if ((opt != NULL) && (strcmp(opt, "") != 0)) {
            switch (p->option) {
            case OPT_CONFIG:
                setIniFile(ocrConfig, opt);
                break;
            }
        }
    }
}

static s32 readFromArgs(s32 argc, const char* argv[], ocrConfig_t * ocrConfig) {
    // Override any env variable with command line option
    s32 cur = 1;
    s32 userArgs = argc;
    char * ocrOptPrefix = "-ocr:";
    s32 ocrOptPrefixLg = strlen(ocrOptPrefix);
    while(cur < argc) {
        const char * arg = argv[cur];
        if (strncmp(ocrOptPrefix, arg, ocrOptPrefixLg) == 0) {
            // This is an OCR option
            const char * ocrArg = arg+ocrOptPrefixLg;
            if (strcmp("cfg", ocrArg) == 0) {
                checkNextArgExists(cur, argc, "cfg");
                setIniFile(ocrConfig, argv[cur+1]);
                argv[cur] = NULL;
                argv[cur+1] = NULL;
                cur++; // skip param
                userArgs-=2;
            } else if (strcmp("version", ocrArg) == 0) {
                printVersion();
                exit(0);
                break;
            } else if (strcmp("help", ocrArg) == 0) {
                printHelp();
                exit(0);
                break;
            }
        }
        cur++;
    }
    return userArgs;
}

static void ocrConfigInit(ocrConfig_t * ocrConfig) {
    ocrConfig->userArgc = 0;
    ocrConfig->userArgv = NULL;
    ocrConfig->iniFile = NULL;
}

// Public methods

// All these externs are in ocr-driver.c.
extern void bringUpRuntime(const char *inifile);
extern void freeUpRuntime();
extern void stopAllPD(ocrPolicyDomain_t *pd);

/**!
 * Setup and starts the OCR runtime.
 * @param ocrConfig Data-structure containing runtime options
 */
void ocrInit(ocrConfig_t * ocrConfig) {
    const char * iniFile = ocrConfig->iniFile;
    ASSERT(iniFile != NULL);
    bringUpRuntime(iniFile);
}

void ocrParseArgs(s32 argc, const char* argv[], ocrConfig_t * ocrConfig) {

    // Zero-ed the ocrConfig
    ocrConfigInit(ocrConfig);

    // First retrieve options from environment variables
    readFromEnv(ocrConfig);

    // Override any env variable with command line args
    s32 userArgs = readFromArgs(argc, argv, ocrConfig);

    // Check for mandatory options
    checkOcrOption(ocrConfig);

    // Pack argument list
    s32 cur = 0;
    s32 head = 0;
    while(cur < argc) {
        if(argv[cur] != NULL) {
            if (cur == head) {
                head++;
            } else {
                argv[head] = argv[cur];
                argv[cur] = NULL;
                head++;
            }
        }
        cur++;
    }
    ocrConfig->userArgc = userArgs;
    ocrConfig->userArgv = (char **) argv;
}

// This function is for x86 for now. We may need to make this a function
// pointer for other platforms
void ocrFinalize() {
    ocrPolicyDomain_t *pd = NULL;
    ocrWorker_t *worker = NULL;
    getCurrentEnv(&pd, &worker, NULL, NULL);
    // We start the current worker. After it starts, it will loop
    // until ocrShutdown is called which will cause the entire PD
    // to stop (including this worker). The currently executing
    // worker then fallthrough from start to finish.
    worker->fcts.start(worker, pd);
    // NOTE: finish blocks until stop has completed
    pd->fcts.finish(pd);
    pd->fcts.destruct(pd);
    freeUpRuntime();
// #ifdef OCR_ENABLE_STATISTICS
//     ocrStatsProcessDestruct(&GfakeProcess);
//     GocrFilterAggregator->destruct(GocrFilterAggregator);
// #endif
}


// TODO: Relying on pthread_yield() below is a hack and needs to go away
// WARNING: The event MUST be sticky. DO NOT WAIT ON A LATCH EVENT!!!
ocrGuid_t ocrWait(ocrGuid_t eventToYieldForGuid) {
    ocrPolicyDomain_t *pd = NULL;
    ocrPolicyMsg_t msg;
    ocrEvent_t *eventToYieldFor = NULL;
    ocrFatGuid_t result;

    getCurrentEnv(&pd, NULL, NULL, NULL);

#define PD_MSG (&msg)
#define PD_TYPE PD_MSG_GUID_INFO
    msg.type = PD_MSG_GUID_INFO | PD_MSG_REQUEST | PD_MSG_REQ_RESPONSE;
    PD_MSG_FIELD(guid.guid) = eventToYieldForGuid;
    PD_MSG_FIELD(guid.metaDataPtr) = NULL;
    PD_MSG_FIELD(properties) = KIND_GUIDPROP | RMETA_GUIDPROP;
    RESULT_PROPAGATE2(pd->fcts.processMessage(pd, &msg, false), ERROR_GUID);
    eventToYieldFor = (ocrEvent_t *)PD_MSG_FIELD(guid.metaDataPtr);
#undef PD_MSG
#undef PD_TYPE

    ASSERT(eventToYieldFor->kind == OCR_EVENT_STICKY_T ||
           eventToYieldFor->kind == OCR_EVENT_IDEM_T);

    do {
        pthread_yield();
        result.guid = ERROR_GUID;
        result = pd->eventFactories[0]->fcts[eventToYieldFor->kind].get(eventToYieldFor);
    } while(result.guid == ERROR_GUID);

    return result.guid;
}

#endif /* ENABLE_OCR_LIB */
