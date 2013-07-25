#include <stdio.h> 
#include <stdlib.h>
#include "callback.h"

memTable *memQueueHead = NULL;
memTable *memQueueTail = NULL;

void addQueue(ocrGuid_t *db, void* addr, u64 len, ocrTask_t *task){
	//fprintf(stderr, "addQueue called\n");
	memTable *element = (memTable*)malloc(sizeof(memTable));
	element->guid = *db;
	element->addr = addr;
	element->len = len;
	if(task)
	  element->task_edt = *task;

	if(memQueueHead == NULL)
	{
		memQueueHead = element;
		memQueueTail = element;
		return;
	}
	else {
		memQueueTail->next = element;
		memQueueTail = element;
	}

}

static bool isInRange(void *table_addr, void *search_addr, u64 len){
	if(search_addr >= table_addr && search_addr <= table_addr + len)
		return true;

	return false;
}

memTable* isAddrPresent(void* addr){
	//fprintf(stderr, "isAddrPresent called\n");
	memTable *ptr;

	ptr = memQueueHead;

	while(ptr != NULL)
	{
		if(isInRange(ptr->addr, addr, ptr->len))
			return ptr;
		else ptr = ptr->next;
	}
	return NULL;
}

void build_addresstable_callback(void){
	printf("address build table called\n");
}

int load_callback(void* address, unsigned long size)
{
	memTable *ptr;  
	//printf("callback load_callback called\n");
	//printf("load address:%p size: %lu\n", address, size);
	ocrTask_t *curTask = NULL;
	deguidify(getCurrentPD(), getCurrentEDT(), (u64*)&curTask, NULL);

	if((ptr = isAddrPresent(address))){
		fprintf(stderr, "\n**load address in range**\n");
		fprintf(stderr, "load addr:%x,  mem_guid:%x, range:(%x, %x), creator_edt_guid:%x, access_edt_guid:%x\n", address, ptr->guid, ptr->addr, ptr->addr+ptr->len, ptr->task_edt, curTask->guid);
	}
	return 0;

}

int store_callback(void* address, unsigned long size)
{
	memTable *ptr;  
	//printf("callback store_callback called\n");
	//printf("store address:%p size: %lu\n", address, size);;
	ocrTask_t *curTask = NULL;
	deguidify(getCurrentPD(), getCurrentEDT(), (u64*)&curTask, NULL);

	if((ptr = isAddrPresent(address))){
		fprintf(stderr, "\n**store address in range**\n");
		fprintf(stderr, "store addr:%x,  mem_guid:%x, range:(%x, %x), creator_edt_guid:%x, access_edt_guid:%x\n", address, ptr->guid, ptr->addr, ptr->addr+ptr->len, ptr->task_edt, curTask->guid);
	}
	return 0;

}
