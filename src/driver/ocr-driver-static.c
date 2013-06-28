#include "ocr-runtime.h"

#include "datablock/datablock-all.h"
#include "policy-domain/policy-domain-all.h"
#include "hc.h"

/**!
 * Initialize the OCR runtime.
 * @param argc number of command line options
 * @param argv Pointer to options
 * @param fnc Number of function pointers (UNUSED)
 * @param funcs array of function pointers to be used as edts (UNUSED)
 *
 * Note: removes OCR options from argc / argv
 */
void ocrInit(int * argc, char ** argv, ocrEdt_t mainEdt) {
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

    gHackTotalMemSize = 512*1024*1024; /* 64 MB default */
    // TODO: Bala to replace with his parsing.
    // This should also generate modified arguments to pass to
    // the first EDT (future) kind of like for FSim.
    // For now I am calling them paramc and paramv
    u32 paramc = 0;
    u64* paramv = NULL;

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
    ocrGuid_t mainEdtTemplate;
    ocrGuid_t mainEdtGuid;

    // TODO: REC: Add the creation once I have a PD
}

int main(int argc, char ** argv) {


    ocrParamList_t* policyDomainFactoryParams = NULL;
    ocrPolicyDomainFactory_t * policyDomainFactory = newPolicyDomainFactory ( policyDomainHc_id, policyDomainFactoryParams );

    ocrParamList_t* taskFactoryParams = NULL;
    ocrTaskFactory_t *taskFactory = newTaskFactoryHc ( taskFactoryParams );

    ocrParamList_t* taskTemplateFactoryParams = NULL;
    ocrTaskTemplateFactory_t *taskTemplateFactory = newTaskTemplateFactoryHc ( taskTemplateFactoryParams );

    ocrParamList_t* dataBlockFactoryParams = NULL;
    ocrDataBlockFactory_t *dbFactory = newDataBlockFactoryRegular ( dataBlockFactoryParams );

    /* this seems not to be ocrParamList_t*'ed yet */
    void* eventFactoryConfigParams = NULL;
    ocrEventFactory_t *eventFactory = newEventFactoryHc ( eventFactoryConfigParams );

    ocrParamList_t* policyContextFactoryParams = NULL;
    ocrPolicyCtxFactory_t *contextFactory = newPolicyContextFactoryHC ( policyContextFactoryParams );

    ocrCost_t *costFunction = NULL;

    ocrPolicyDomain_t * rootPolicy = policyDomainFactory->instantiate(
            policyDomainFactory /*factory*/, NULL /*configuration*/,
            1 /*schedulerCount*/, 8/*workerCount*/, 8 /*computeCount*/,
            8 /*workpileCount*/, 1 /*allocatorCount*/, 1 /*memoryCount*/,
            taskFactory, taskTemplateFactory, dbFactory,
            eventFactory, contextFactory, costFunction
           );

	return 0;
}
