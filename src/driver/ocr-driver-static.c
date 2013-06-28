#include "ocr-runtime.h"

#include "datablock/datablock-all.h"
#include "policy-domain/policy-domain-all.h"
#include "hc.h"

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
