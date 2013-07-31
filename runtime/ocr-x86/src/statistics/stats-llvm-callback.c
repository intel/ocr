/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#ifdef OCR_ENABLE_STATISTICS

#include "ocr-task.h"
#include "ocr-policy-domain.h"
#include "ocr-policy-domain-getter.h"
#include "statistics/stats-llvm-callback.h"

#include <stdio.h> 
#include <stdlib.h>

memTable_t *memQueueHead = NULL;
memTable_t *memQueueTail = NULL;

static bool isInRange(void *tableAddr, void *searchAddr, u64 len){
    return (searchAddr >= tableAddr && searchAddr <= tableAddr + len);
}

void _statsAddQueue(ocrGuid_t *db, void* addr, u64 len, ocrTask_t *task) {
    //fprintf(stderr, "_statsAddQueue called\n");
    memTable_t *element = (memTable_t*)malloc(sizeof(memTable_t));
    element->guid = *db;
    element->addr = addr;
    element->len = len;
    if(task)
        element->taskEdt = *task;

    if(memQueueHead == NULL) {
        memQueueHead = element;
        memQueueTail = element;
        return;
    } else {
        memQueueTail->next = element;
        memQueueTail = element;
    }
}


memTable_t* _statsIsAddrPresent(void* addr){
    //fprintf(stderr, "_statsIsAddrPresent called\n");
    memTable_t *ptr;

    ptr = memQueueHead;

    while(ptr != NULL)
    {
        if(isInRange(ptr->addr, addr, ptr->len))
            return ptr;
        else ptr = ptr->next;
    }
    return NULL;
}

u32 _statsLoadCallback(void* address, u64 size)
{
    memTable_t *ptr;  
    //printf("callback _statsLoadCallback called\n");
    //printf("load address:%p size: %lu\n", address, size);
    ocrTask_t *curTask = NULL;
    deguidify(getCurrentPD(), getCurrentEDT(), (u64*)&curTask, NULL);

    if((ptr = _statsIsAddrPresent(address))){
        fprintf(stderr, "\n**load address in range**\n");
        fprintf(stderr, "load addr:%x, mem_guid:%x, range:(%x, %x), "
                    "creator_edt_guid:%x, access_edt_guid:%x\n",
                    address, ptr->guid, ptr->addr, ptr->addr+ptr->len,
                    ptr->taskEdt, curTask->guid);
    }
    return 0;

}

u32 _statsStoreCallback(void* address, u64 size)
{
    memTable_t *ptr;  
    //printf("callback _statsStoreCallback called\n");
    //printf("store address:%p size: %lu\n", address, size);;
    ocrTask_t *curTask = NULL;
    deguidify(getCurrentPD(), getCurrentEDT(), (u64*)&curTask, NULL);

    if((ptr = _statsIsAddrPresent(address))){
        fprintf(stderr, "\n**store address in range**\n");
        fprintf(stderr, "store addr:%x,  mem_guid:%x, range:(%x, %x), "
                    "creator_edt_guid:%x, access_edt_guid:%x\n",
                    address, ptr->guid, ptr->addr, ptr->addr+ptr->len,
                    ptr->taskEdt, curTask->guid);
    }
    return 0;

}

void statsBuildAddressTableCallback(void){
    printf("address build table called\n");
}

#endif /* OCR_ENABLE_STATISTICS */
