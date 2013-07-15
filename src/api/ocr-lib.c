/* Copyright (c) 2012, Rice University

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1.  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2.  Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
   3.  Neither the name of Intel Corporation
   nor the names of its contributors may be used to endorse or
   promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "debug.h"
#include "machine-description/ocr-machine.h"
#include "ocr-lib.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "ocr-types.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

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

    // At this point the runtime is up
#ifdef OCR_ENABLE_STATISTICS
    GocrFilterAggregator = NEW_FILTER(filedump);
    GocrFilterAggregator->create(GocrFilterAggregator, NULL, NULL);

    // HUGE HUGE Hack
    ocrStatsProcessCreate(&GfakeProcess, 0);
#endif
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

void ocrFinalize() {
    ocrPolicyDomain_t* masterPD = getMasterPD();
    stopAllPD(masterPD);
    masterPD->destruct(masterPD);
    freeUpRuntime();
    //TODO we need to start by stopping the master PD which
    //controls stopping down PD located on the same machine.
// #ifdef OCR_ENABLE_STATISTICS
//     ocrStatsProcessDestruct(&GfakeProcess);
//     GocrFilterAggregator->destruct(GocrFilterAggregator);
// #endif
}

ocrGuid_t ocrWait(ocrGuid_t eventToYieldForGuid) {
    ocrPolicyCtx_t * ctx = getCurrentWorkerContext();
    ocrPolicyDomain_t * pd = ctx->PD;

    ocrGuid_t workerGuid = ctx->sourceObj;
    ocrGuid_t yieldingEdtGuid = getCurrentEDT();
    // The guid returned by eventGuid on completion
    ocrGuid_t returnGuid;
    // This call should return only when the event 'eventToYieldForGuid' is satisfied
    pd->waitForEvent(pd, workerGuid, yieldingEdtGuid, eventToYieldForGuid, &returnGuid, ctx);

    return returnGuid;
}
