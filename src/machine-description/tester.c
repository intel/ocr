#include <stdio.h>
#include <string.h>
#include <iniparser.h>

#include <ocr-allocator.h>
#include <ocr-comp-platform.h>
#include <ocr-mem-platform.h>
#include <ocr-mem-target.h>
#include <ocr-datablock.h>
#include <ocr-event.h>

//#include <ocr-comp-target.h>
//#include <ocr-worker.h>
//#include <ocr-workpile.h>

//#include <allocator/allocator-all.h>

#define INI_GETINT(KEY, VAR, DEF) VAR = (int) iniparser_getint(dict, KEY, DEF); if (VAR==DEF){ printf("Key %s not found or invalid!\n", KEY); }
#define INI_GETSTR(KEY, VAR, DEF) VAR = (char *) iniparser_getstring(dict, KEY, DEF); if (!strcmp(VAR, DEF)){ printf("Key %s not found or invalid!\n", KEY); }
#define TO_ENUM(VAR, STR, TYPE, TYPESTR, COUNT) {int kount; for (kount=0; kount<COUNT; kount++) {if (!strcmp(STR, TYPESTR[kount])) VAR=(TYPE)kount;} }

void dump_allocator_fact(paramListAllocatorFact_t *allocFact)
{
    if (allocFact == NULL) return;
    printf("AllocFact: %s\n", allocFact->base.misc);
}

void dump_allocator_inst(paramListAllocatorInst_t *allocInst)
{
    if (allocInst == NULL) return;
    printf("AllocInst: %s; %d\n", allocInst->base.misc, allocInst->size);
}

paramListAllocatorFact_t **allocatorFact;
paramListAllocatorInst_t **allocatorInst;
paramListCompPlatformFact_t **cplatformFact;
paramListCompPlatformInst_t **cplatformInst;
paramListMemPlatformFact_t **mplatformFact;
paramListMemPlatformInst_t **mplatformInst;
paramListMemTargetFact_t **mtargetFact;
paramListMemTargetInst_t **mtargetInst;
//paramListWorkerFact_t **workerFact;
//paramListWorkerInst_t **workerInst;
//paramListWorkpileFact_t **workpileFact;
//paramListWorkpileInst_t **workpileInst;
//paramListSchedulerFact_t **schedulerFact;
//paramListSchedulerInst_t **schedulerInst;
//paramListCompTargetFact_t **ctargetFact;
//paramListCompTargetInst_t **ctargetInst;
paramListDataBlockFact_t **dbFact;
paramListDataBlockInst_t **dbInst;
paramListEventFact_t **edtFact;
//paramListEventInst_t **edtInst;

int main(int argc, char *argv[])
{
     char **seckey;
     int i;
     char *typestr;
     char key[64];
     dictionary *dict = iniparser_load(argv[1]);

// Allocator

     int type_count, instance_count;

     INIT_PARAM_LIST(allocatorFact, paramListAllocatorFact_t);
     INI_GETINT ("Allocator:types", type_count, -1);
     printf("Allocator types = %d\n", type_count);

     allocatorFact = (paramListAllocatorFact_t **)malloc(type_count * sizeof(paramListAllocatorFact_t *));

     for (i = 0; i < type_count; i++) {
	ALLOC_PARAM_LIST(allocatorFact[i], paramListAllocatorFact_t);

	snprintf(key, 64, "%sType%d:misc", "Allocator", i);
	INI_GETSTR (key, typestr, " ");
	if(strcmp(typestr,"")) allocatorFact[i]->base.misc = strdup(typestr); else allocatorFact[i]->base.misc = NULL;

	snprintf(key, 64, "%sType%d:name", "Allocator", i);
	INI_GETSTR (key, typestr, "");
     }

     INI_GETINT ("Allocator:count", instance_count, -1);
     printf("Allocator instances = %d\n", instance_count);

     allocatorInst = (paramListAllocatorInst_t **)malloc(instance_count * sizeof(paramListAllocatorInst_t *));

     for (i = 0; i < instance_count; i++) {
        ALLOC_PARAM_LIST(allocatorInst[i], paramListAllocatorInst_t);

	snprintf(key, 64, "%s%d:type", "allocator", i);
	INI_GETSTR (key, typestr, " ");
	if(strcmp(typestr,"")) allocatorInst[i]->base.misc = strdup(typestr); else allocatorInst[i]->base.misc = NULL;

	snprintf(key, 64, "%s%d:size", "allocator", i);
	INI_GETINT (key, allocatorInst[i]->size, -1);

     }

     for(i = 0; i<type_count; i++) dump_allocator_fact (allocatorFact[i]);
     for(i = 0; i<instance_count; i++) dump_allocator_inst (allocatorInst[i]);

     INIT_PARAM_LIST(mtargetFact, paramListMemTargetFact_t);
     INI_GETINT ("MemTarget:types", type_count, -1);

     printf("Memtarget types = %d\n", type_count);

     mtargetFact = (paramListMemTargetFact_t **)malloc(type_count * sizeof(paramListMemTargetFact_t *));

     for (i = 0; i < type_count; i++) {
	ALLOC_PARAM_LIST(mtargetFact[i], paramListMemTargetFact_t);

	snprintf(key, 64, "%sType%d:misc", "MemTarget", i);
	if(strcmp(typestr,"")) mtargetFact[i]->base.misc = strdup(typestr); else mtargetFact[i]->base.misc = NULL;

	snprintf(key, 64, "%sType%d:name", "MemTarget", i);
	INI_GETSTR (key, typestr, "");
     }

     // TODO: Missing, logic to read in memory target & associate with allocator. This would be done in two passes
     // In the first pass, we read in the config items & build the paramLists
     // In the second pass, we build the references between the paramLists
     // Finally we're ready to pass them on to the constructors elsewhere


     return 0;
}
