/*
 * This file is subject to the license agreement located in the file LICENSE
 * and cannot be distributed without it. This notice cannot be
 * removed or modified.
 */

#include "ocr-config.h"
#ifdef ENABLE_SAL_LINUX

#include "sal/linux/linux-sal.h"
#include "debug.h"
#include "ocr-policy-domain.h"
#include "ocr-task.h"
#include "ocr-sal.h"
#include "ocr-sysboot.h"
#include "ocr-worker.h"

#include <stdlib.h>
#include <stdio.h>

void linuxSalTeardown(ocrPolicyDomain_t *pd) {
    // TODO: Implement
}

void (*teardown)(ocrPolicyDomain_t*) = &linuxSalTeardown;
void (*getCurrentEnv)(ocrPolicyDomain_t**, ocrWorker_t**,
                      ocrTask_t **, ocrPolicyMsg_t*) = NULL; // This is set by the comp-platform

void linuxSalDestruct(ocrSal_t* self) {
    runtimeChunkFree((u64)self, NULL);
}

void linuxSalStart(ocrSal_t *self, ocrPolicyDomain_t *pd) {
    // No GUID for the system. Do we need one?
    self->pd = pd;
}

void linuxSalStop(ocrSal_t *self) {
    // Nothing to do
}

void linuxSalFinish(ocrSal_t *self) {
    // Nothing to do
}

void linuxSalExit(ocrSal_t *self, u64 errorCode) {
    exit(errorCode);
}

void linuxSalAbort(ocrSal_t *self) {
    abort();
}

void linuxSalPrint(ocrSal_t *self, const char* str, u64 length) {
    ASSERT(str[length] == '\0');
    printf("%s", str);
}

u64 linuxSalWrite(ocrSal_t *self, const char* str, u64 length, u64 id) {
    return 0;
}

u64 linuxSalRead(ocrSal_t *self, char *str, u64 length, u64 id) {
    return 0;
}

ocrSal_t * newSalLinux(ocrSalFactory_t * factory, ocrParamList_t* perInstance) {
    ocrSalLinux_t * sal = (ocrSalLinux_t*)runtimeChunkAlloc(sizeof(ocrSalLinux_t), NULL);

    sal->base.pd = NULL;
    sal->base.fcts = factory->salFcts;
        
    return (ocrSal_t*)sal;
}

/******************************************************/
/* OCR SAL LINUX FACTORY                              */
/******************************************************/
static void destructSalFactoryLinux(ocrSalFactory_t *factory) {
    runtimeChunkFree((u64)factory, NULL);
}

ocrSalFactory_t *newSalFactoryLinux(ocrParamList_t *perType) {
    ocrSalFactory_t *base = (ocrSalFactory_t*)
        runtimeChunkAlloc(sizeof(ocrSalFactoryLinux_t), (void *)1);
    base->instantiate = &newSalLinux;
    base->destruct = &destructSalFactoryLinux;
    base->salFcts.destruct = FUNC_ADDR(void (*)(ocrSal_t*), linuxSalDestruct);
    base->salFcts.start = FUNC_ADDR(void (*)(ocrSal_t*, ocrPolicyDomain_t*), linuxSalStart);
    base->salFcts.stop = FUNC_ADDR(void (*)(ocrSal_t*), linuxSalStop);
    base->salFcts.finish = FUNC_ADDR(void (*)(ocrSal_t*), linuxSalFinish);
    base->salFcts.exit = FUNC_ADDR(void (*)(ocrSal_t*, u64), linuxSalExit);
    base->salFcts.abort = FUNC_ADDR(void (*)(ocrSal_t*), linuxSalAbort);
    base->salFcts.print = FUNC_ADDR(void (*)(ocrSal_t*, const char*, u64), linuxSalPrint);
    base->salFcts.write = FUNC_ADDR(u64 (*)(ocrSal_t*, const char*, u64, u64), linuxSalWrite);
    base->salFcts.read = FUNC_ADDR(u64 (*)(ocrSal_t*, char*, u64, u64), linuxSalRead);

    return base;
}

#endif /* ENABLE_SAL_LINUX */
