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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ocr-edt.h"
#include "ocr-db.h"
#include "ocr.h"

#include "ocr-runtime.h"
#include "ocr-config.h"
#include "ocr-guid.h"
#include "ocr-utils.h"
#include "debug.h"

#ifdef OCR_ENABLE_STATISTICS
#include "ocr-stat-user.h"
#endif

extern void hack();

// Forward declaration
void ocrStop();

// //TODO we should have an argument option parsing library
// /**!
//  * Utility function to remove ocr arguments before handing argv
//  * over to the user program
//  */
// static void shift_arguments(int * argc, char ** argv, int start_offset, int shift_offset) {
//     int i = start_offset;
//     int j = shift_offset;
//     while ( j < *argc) {
//         argv[i++] = argv[j++];
//     }

//     *argc = (*argc - (shift_offset-start_offset));
// }

// /**!
//  * Check if we have a machine description passed as argument
//  */
// static char * parseOcrOptions_MachineDescription(int * argc, char ** argv) {
//     int i = 0;
//     char * md_file = NULL;
//     while(i < *argc) {
//         if (strcmp(argv[i], "-md") == 0) {
//             md_file = argv[i+1];
//             shift_arguments(argc, argv, i, i+2);
//         }
//         i++;
//     }
//     return md_file;
// }

/**!
 * Initialize the OCR runtime.
 * @param argc number of command line options
 * @param argv Pointer to options
 * @param fnc Number of function pointers (UNUSED)
 * @param funcs array of function pointers to be used as edts (UNUSED)
 *
 * Note: removes OCR options from argc / argv
 */
void ocrInit(int argc, char ** argv, ocrEdt_t mainEdt, bool createFinishEdt) {
    // REC: Hack, for now we just initialize the factories here.
    // See if we need to move some into policy domains and how to
    // initialize them
    // Do this first to make sure that stuff that depends on it works :)

#ifdef OCR_ENABLE_STATISTICS
    GocrFilterAggregator = NEW_FILTER(filedump);
    GocrFilterAggregator->create(GocrFilterAggregator, NULL, NULL);

    // HUGE HUGE Hack
    ocrStatsProcessCreate(&GfakeProcess, 0);
#endif

    /*
    char * md_file = parseOcrOptions_MachineDescription(argc, argv);
    if ( md_file != NULL && !strncmp(md_file,"fsim",5) ) {
        ocrInitPolicyModel(OCR_POLICY_MODEL_FSIM, md_file);
    } else if ( md_file != NULL && !strncmp(md_file,"thor",4) ) {
        ocrInitPolicyModel(OCR_POLICY_MODEL_THOR, md_file);
    } else {
        ocrInitPolicyModel(OCR_POLICY_MODEL_FLAT, md_file);
    }
    */

    // Create and schedule the main EDT
    // ocrGuid_t mainEdtTemplate;
    // ocrGuid_t mainEdtGuid;

    // Parse parameters. The idea is to extract the ones relevant
    // to the runtime and pass all the other ones down to the
    // mainEdt

    // TODO: Add code recognizing useful options. For now, pass
    // everything in a heavy handed way so that it can handle
    // the more complex case

    ASSERT(argc < 64); // For now
    u32 i;
    u64* offsets = (u64*)malloc(argc*sizeof(u64));
    u64 argsUsed = 0ULL;
    u64 totalLength = 0;
    u32 maxArg = 0;
    // Gets all the possible offsets
    for(i = 1; i < argc; ++i) {
        // If the argument should be passed down
        offsets[maxArg++] = totalLength*sizeof(char);
        totalLength += strlen(argv[i]) + 1; // +1 for the NULL terminating char
        argsUsed |= (1ULL<<(63-i));
    }
    --maxArg;
    // Create the datablock containing the parameters
    ocrGuid_t dbGuid;
    void* dbPtr;

    ocrDbCreate(&dbGuid, &dbPtr, totalLength + (maxArg + 1)*sizeof(u64),
                DB_PROP_NONE, NULL_GUID, NO_ALLOC);

    // Copy in the values to the data-block. The format is as follows:
    // - First 4 bytes encode the number of arguments (u64) (called argc)
    // - After that, an array of argc u64 offsets is encoded.
    // - The strings are then placed after that at the offsets encoded
    //
    // The use case is therefore as follows:
    // - Cast the DB to a u64* and read the number of arguments and
    //   offsets (or whatever offset you need)
    // - Case the DB to a char* and access the char* at the offset
    //   read. This will be a null terminated string.

    // Copy the metadata
    u64* dbAsU64 = (u64*)dbPtr;
    dbAsU64[0] = (u64)maxArg;
    u64 extraOffset = (maxArg + 1)*sizeof(u64);
    for(i = 0; i < maxArg; ++i) {
        dbAsU64[i+1] = offsets[i] + extraOffset;
    }

    // Copy the actual arguments
    char* dbAsChar = (char*)dbPtr;
    while(argsUsed) {
        u32 pos = fls64(argsUsed);
        argsUsed &= ~(1ULL<<pos);
        strcpy(dbAsChar + extraOffset + offsets[63 - pos], argv[63 - pos]);
    }

    // We now create the EDT and launch it
    ocrGuid_t edtTemplateGuid, edtGuid;
    ocrEdtTemplateCreate(&edtTemplateGuid, mainEdt, 0, 1);
    if(createFinishEdt) {
        ocrGuid_t outputEvt;
        ocrEdtCreate(&edtGuid, edtTemplateGuid, 0, /* paramv = */ NULL,
                     /* depc = */ 1, /* depv = */ &dbGuid,
                     EDT_PROP_FINISH, NULL_GUID, &outputEvt);
        // TODO: Re-add when ocrWait is implemented
//          ocrWait(outputEvt);
        ocrShutdown();
    } else {
        ocrEdtCreate(&edtGuid, edtTemplateGuid, 0, /* paramv = */ NULL,
                     /* depc = */ 1, /* depv = */ &dbGuid,
                     EDT_PROP_NONE, NULL_GUID, NULL);
    }

    ocrStop();
}

int __attribute__ ((weak)) main(int argc, const char* argv[]) {
//    ocrInit(argc, argv, &mainEdt, false);
    hack();
    return 0;
}

// static void recursive_policy_finish_helper ( ocrPolicyDomain_t* curr ) {
//     if ( curr ) {
//         int index = 0; // successor index
//         for ( ; index < curr->n_successors; ++index ) {
//             recursive_policy_finish_helper(curr->successors[index]);
//         }
//         curr->finish(curr);
//     }
// }

// static void recursive_policy_stop_helper ( ocrPolicyDomain_t* curr ) {
//     if ( curr ) {
//         int index = 0; // successor index
//         for ( ; index < curr->n_successors; ++index ) {
//             recursive_policy_stop_helper(curr->successors[index]);
//         }
//         curr->stop(curr);
//     }
// }

// static void recursive_policy_destruct_helper ( ocrPolicyDomain_t* curr ) {
//     if ( curr ) {
//         int index = 0; // successor index
//         for ( ; index < curr->n_successors; ++index ) {
//             recursive_policy_destruct_helper(curr->successors[index]);
//         }
//         curr->destruct(curr);
//     }
// }

// static inline void unravel () {
//     // current root policy index
//     int index = 0;

//     for ( index = 0; index < n_root_policy_nodes; ++index ) {
//         recursive_policy_finish_helper(root_policies[index]);
//     }

//     for ( index = 0; index < n_root_policy_nodes; ++index ) {
//         recursive_policy_stop_helper(root_policies[index]);
//     }

//     for ( index = 0; index < n_root_policy_nodes; ++index ) {
//         recursive_policy_destruct_helper(root_policies[index]);
//     }
//     // TODO this would be destroyed as part of the root policy
//     globalGuidProvider->destruct(globalGuidProvider);
//     free(root_policies);
// }

// TODO sagnak everything below is DUMB and RIDICULOUS and
// will have to be undone and done again

struct _ocrPolicyDomainLinkedListNode;
typedef struct _ocrPolicyDomainLinkedListNode {
    ocrPolicyDomain_t* pd;
    struct _ocrPolicyDomainLinkedListNode* next;
} ocrPolicyDomainLinkedListNode;

// walkthrough the linked list and return TRUE if instance exists
int isMember ( ocrPolicyDomainLinkedListNode *dummyHead, ocrPolicyDomainLinkedListNode *tail, ocrPolicyDomain_t* instance ) {
    ocrPolicyDomainLinkedListNode* curr = dummyHead->next;
    for ( ; NULL != curr && curr->pd != instance; curr = curr -> next ) {
    }
    return NULL != curr;
}

void recurseBuildDepthFirstSpanningTreeLinkedList ( ocrPolicyDomainLinkedListNode *dummyHead, ocrPolicyDomainLinkedListNode *tail, ocrPolicyDomain_t* currPD ) {
    ocrPolicyDomainLinkedListNode *currNode = (ocrPolicyDomainLinkedListNode*) malloc(sizeof(ocrPolicyDomainLinkedListNode));
    currNode -> pd = currPD;
    currNode -> next = NULL;
    tail -> next = currNode;
    tail = currNode;

    u64 neighborCount = currPD->neighborCount;
    u64 i = 0;
    for ( ; i < neighborCount; ++i ) {
        ocrPolicyDomain_t* currNeighbor = currPD->neighbors[i];
        if ( !isMember(dummyHead,tail,currNeighbor) ) {
            recurseBuildDepthFirstSpanningTreeLinkedList(dummyHead, tail, currNeighbor);
        }
    }
}

ocrPolicyDomainLinkedListNode *buildDepthFirstSpanningTreeLinkedList ( ocrPolicyDomain_t* currPD ) {

    ocrPolicyDomainLinkedListNode *dummyHead = (ocrPolicyDomainLinkedListNode*) malloc(sizeof(ocrPolicyDomainLinkedListNode));
    ocrPolicyDomainLinkedListNode *tail = dummyHead;
    dummyHead -> pd = NULL;
    dummyHead -> next = NULL;

    recurseBuildDepthFirstSpanningTreeLinkedList(dummyHead, tail, currPD);
    ocrPolicyDomainLinkedListNode *head = dummyHead->next;
    free(dummyHead);
    return head;
}

void destructLinkedList ( ocrPolicyDomainLinkedListNode* head ) {
    ocrPolicyDomainLinkedListNode *curr = head;
    ocrPolicyDomainLinkedListNode *next = NULL;
    while ( NULL != curr ) {
        next = curr->next;
        free(curr);
        curr = next;
    }
}
void linearTraverseFinish( ocrPolicyDomainLinkedListNode* curr ) {
    ocrPolicyDomainLinkedListNode *head = curr;
    for ( ; NULL != curr; curr = curr -> next ) {
        ocrPolicyDomain_t* pd = curr->pd;
        pd->finish(pd);
    }
    destructLinkedList(head);
}

void linearTraverseStop ( ocrPolicyDomainLinkedListNode* curr ) {
    ocrPolicyDomainLinkedListNode *head = curr;
    for ( ; NULL != curr; curr = curr -> next ) {
        ocrPolicyDomain_t* pd = curr->pd;
        pd->stop(pd);
    }
    destructLinkedList(head);
}

void ocrShutdown() {
    ocrPolicyDomain_t* currPD = getCurrentPD();
    ocrPolicyDomainLinkedListNode * spanningTreeHead = buildDepthFirstSpanningTreeLinkedList(currPD); //N^2
    linearTraverseFinish(spanningTreeHead);
}

void ocrStop() {
    ocrPolicyDomain_t* masterPD = getMasterPD();
    ocrPolicyDomainLinkedListNode * spanningTreeHead = buildDepthFirstSpanningTreeLinkedList(masterPD); //N^2
    linearTraverseStop(spanningTreeHead);
    //TODO we need to start by stopping the master PD which
    //controls stopping down PD located on the same machine.
// #ifdef OCR_ENABLE_STATISTICS
//     ocrStatsProcessDestruct(&GfakeProcess);
//     GocrFilterAggregator->destruct(GocrFilterAggregator);
// #endif
}
