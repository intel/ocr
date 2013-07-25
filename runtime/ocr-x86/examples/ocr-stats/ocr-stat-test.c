/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ocr.h"

#define FLAGS 0xdead

ocrGuid_t mainEdt ( u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    printf("Hello !\n");
    void *mem_db;
    ocrGuid_t mem_guid, mem_affinity;
    ocrDbCreate( &mem_guid, &mem_db, 100, FLAGS, mem_affinity, NO_ALLOC );

    int i;
    char *buf = (char*)malloc(10*sizeof(char));
    for(i = 0; i < 10; i++)
	    buf[i] = i;

    int *ptr = (int*)mem_db;
    ptr[0] = 109;
    ptr[1] = 101;

    int j = ptr[0];
    ocrShutdown();
    return 0;
}
