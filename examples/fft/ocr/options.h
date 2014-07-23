/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdlib.h>
#include <getopt.h>
#include <cmath>

#include "ocr.h"

void printHelp(char *argv[], bool supportSerialBlocks) {
    PRINTF("\n");
    if(supportSerialBlocks) {
        PRINTF("%s -n N [--serial-block|-s SIZE] [--check|-c] [-i iterations] [--verbose|-v]\n",argv[0]);
        PRINTF("    [--print-result|-p]\n");
        PRINTF("    --serial-block|-s SIZE  Divide the workload into minimum size chunks of 2^SIZE elements. Default=16384.\n");
    } else {
        PRINTF("%s -n N  [--check|-c] [-i iterations] [--verbose|-v] [--print-result|-p]\n",argv[0]);
    }
    PRINTF("    -n N                    Computes a matrix with 2^N elements.\n");
    PRINTF("    --check|-c              Checks the computed results against serial baseline.\n");
    PRINTF("    -i iterations           Runs the given number of iterations (default 1).\n");
    PRINTF("    --verbose|-v            Print out more information.\n");
    PRINTF("    --print-result|-p       Print input and result of computation.\n");
}

// Parses the command line options, returning true if a legal configuration
// was provided.
bool parseOptions(int argc, char *argv[], u64 *N, bool *verify, u64 *iterations,
                  bool *verbose, bool *printResults, u64 *serialBlockSize) {
    opterr = 0;
    char c;
    char *buffer = NULL;
    *verify = false;
    *verbose = false;
    *printResults = false;
    *N = -1;
    *iterations = 1;


    while(1) {
        // For processing long options
        int option_index = 0;

        static struct option long_options[] = {
            {"verbose", no_argument, NULL, 'v'},
            {"print-result", no_argument, NULL, 'p'},
            {"check", no_argument, NULL, 'c'},
            {"serial-block", no_argument, NULL, 's'},
            {0,0,0,0}
        };

        c = getopt_long(argc, argv, "ci:n:s:vp", long_options, &option_index);
        if(c == -1)
            break;

        switch(c) {
            case 'c':
                *verify = true;
                break;
            case 'i':
                *iterations = atoi(optarg);
                break;
            case 'n':
                *N = pow(2,atoi(optarg));
                break;
            case 'v':
                *verbose = true;
                break;
            case 'p':
                *printResults = true;
                break;
            case 's':
                *serialBlockSize = pow(2,atoi(optarg));
                break;
            case '?':
                if(optopt == 'n' || optopt == 'i' || optopt == 's') {
                    PRINTF("Option -%c requires an argument.\n",optopt);
                } else {
                    PRINTF("Unknown option -%c.\n",optopt);
                }
        }
    }
    return (*N != -1);
}
