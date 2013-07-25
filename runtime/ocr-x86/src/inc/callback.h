#define ELS_USER_SIZE 0
#include "ocr.h"
#include "ocr-allocator.h"
#include "ocr-datablock.h"
#include "ocr-db.h"
#include "ocr-policy-domain-getter.h"
#include "ocr-policy-domain.h"
#include "debug.h"

typedef struct _memTable{
	ocrGuid_t guid;
	void *addr;
	u64 len;
	ocrTask_t task_edt;
	struct _memTable *next;
}memTable;

void addQueue(ocrGuid_t *db, void* addr, u64 len, ocrTask_t *task);
memTable* isAddrPresent(void* addr);
int load_callback(void* address, unsigned long size); 
int store_callback(void* address, unsigned long size); 
void build_addresstable_callback(void);
