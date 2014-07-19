/*
 * fib.c
 * Does not use finish EDTs
 * Originally written in Feb 2012 by Justin Teller
 * Modified for OCR 0.9 by Romain Cledat
 */

#include "ocr.h"
#include "ocr-std.h"

ocrGuid_t complete(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u64 arg = (u64)paramv[0];

    ocrGuid_t inDep;
    u32 in1, in2;
    u32 out;

    inDep = (ocrGuid_t)arg;

    /* When we run, we got our inputs from fib(n-1) and fib(n-2) */
    in1 = *(u32*)depv[0].ptr;
    in2 = *(u32*)depv[1].ptr;
    out = *(u32*)depv[2].ptr;
    PRINTF("Done with %d (%d + %d)\n", out, in1, in2);
    /* we return our answer in the 3rd db passed in as an argument */
    *((u32*)(depv[2].ptr)) = in1 + in2;

    /* The app is done with the answers from fib(n-1) and fib(n-2) */
    ocrDbDestroy(depv[0].guid);
    ocrDbDestroy(depv[1].guid);

    /* and let our parent's completion know we're done with fib(n) */
    ocrEventSatisfy(inDep, depv[2].guid);

    return NULL_GUID;
}

ocrGuid_t fibEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    void* ptr;
    ocrGuid_t inDep;
    ocrGuid_t fib0, fib1, comp;
    ocrGuid_t fibDone[2];
    ocrGuid_t fibArg[2];

    inDep = (ocrGuid_t)paramv[0];

    u32 n = *(u32*)(depv[0].ptr);
    PRINTF("Starting fibEdt(%u)\n", n);
    if (n < 2) {
        PRINTF("In fibEdt(%d) -- done (sat %lx)\n", n, inDep);
        ocrEventSatisfy(inDep, depv[0].guid);
        return NULL_GUID;
    }
    PRINTF("In fibEdt(%d) -- spawning children\n", n);

    /* create the completion EDT and pass it the in/out argument as a dependency */
    /* create the EDT with the done_event as the argument */
    {
        u64 paramv = (u64)inDep;

        ocrGuid_t templateGuid;
        ocrEdtTemplateCreate(&templateGuid, complete, 1, 3);
        ocrEdtCreate(&comp, templateGuid, 1, &paramv, 3, NULL, EDT_PROP_NONE,
                     NULL_GUID, NULL);
        ocrEdtTemplateDestroy(templateGuid);
    }
    PRINTF("In fibEdt(%u) -- spawned complete EDT GUID 0x%llx\n", n, (u64)comp);
    ocrAddDependence(depv[0].guid, comp, 2, DB_DEFAULT_MODE);

    /* create the events that the completion EDT will "wait" on */
    ocrEventCreate(&fibDone[0], OCR_EVENT_ONCE_T, TRUE);
    ocrEventCreate(&fibDone[1], OCR_EVENT_ONCE_T, TRUE);
    ocrAddDependence(fibDone[0], comp, 0, DB_DEFAULT_MODE);
    ocrAddDependence(fibDone[1], comp, 1, DB_DEFAULT_MODE);
    /* allocate the argument to pass to fib(n-1) */

    ocrDbCreate(&fibArg[0], (void**)&ptr, sizeof(u32), DB_PROP_NONE, NULL_GUID, NO_ALLOC);
    PRINTF("In fibEdt(%u) -- created arg DB GUID 0x%llx\n", n, fibArg[0]);
    *((u32*)ptr) = n-1;
    /* sched the EDT, passing the fibDone event as it's argument */
    {
        u64 paramv = (u64)fibDone[0];
        ocrGuid_t depv = fibArg[0];

        ocrGuid_t templateGuid;
        ocrEdtTemplateCreate(&templateGuid, fibEdt, 1, 1);
        ocrEdtCreate(&fib0, templateGuid, 1, &paramv, 1, &depv, EDT_PROP_NONE,
                     NULL_GUID, NULL);
        ocrEdtTemplateDestroy(templateGuid);
    }

    PRINTF("In fibEdt(%u) -- spawned first sub-part EDT GUID 0x%llx\n", n, fib0);
    /* then do the exact same thing for n-2 */
    ocrDbCreate(&fibArg[1], (void**)&ptr, sizeof(u32), DB_PROP_NONE, NULL_GUID, NO_ALLOC);
    PRINTF("In fibEdt(%u) -- created arg DB GUID 0x%llx\n", n, fibArg[1]);
    *((u32*)ptr) = n-2;
    {
        u64 paramv = (u64)fibDone[1];
        ocrGuid_t depv = fibArg[1];

        ocrGuid_t templateGuid;
        ocrEdtTemplateCreate(&templateGuid, fibEdt, 1, 1);
        ocrEdtCreate(&fib1, templateGuid, 1, &paramv, 1, &depv, EDT_PROP_NONE,
                     NULL_GUID, NULL);
        ocrEdtTemplateDestroy(templateGuid);
    }
    PRINTF("In fibEdt(%u) -- spawned first sub-part EDT GUID 0x%llx\n", n, fib1);

    PRINTF("Returning from fibEdt(%u)\n", n);
    return NULL_GUID;

}

ocrGuid_t absFinal(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    u32 ans;
    ans = *(u32*)depv[0].ptr;
    VERIFY(ans == 55, "Totally done: answer is %d\n", ans);
    ocrShutdown();
    ocrDbDestroy(depv[0].guid);

    return NULL_GUID;
}

/* just define the main EDT function */
ocrGuid_t mainEdt(u32 paramc, u64* paramv, u32 depc, ocrEdtDep_t depv[]) {
    PRINTF("Starting mainEdt\n");

    ocrGuid_t fibC, totallyDoneEvent, absFinalEdt, templateGuid;
    {
        ocrGuid_t templateGuid;
        ocrEdtTemplateCreate(&templateGuid, absFinal, 0, 1);
        PRINTF("Created template and got GUID 0x%llx\n", templateGuid);
        ocrEdtCreate(&absFinalEdt, templateGuid, 0, NULL, 1, NULL, EDT_PROP_NONE,
                     NULL_GUID, NULL);
        PRINTF("Created ABS EDT and got  GUID 0x%llx\n", absFinalEdt);
        ocrEdtTemplateDestroy(templateGuid);
    }
    /* for right now, the commandline doesn't work */
    u32 argc = getArgc(depv[0].ptr);
    PRINTF("Got argc %d\n", argc);
    /*if(argc != 2) {
        PRINTF("Usage: fib <num>\n");
        ocrShutdown();
        return NULL_GUID;
        }*/
    int input = 10; // Replace with atoi of argv[1]

    /* create a db for the results */
    ocrGuid_t fibArg;
    u32* res;

    PRINTF("Before 1st DB create\n");
    ocrDbCreate(&fibArg, (void**)&res, sizeof(u32), DB_PROP_NONE, NULL_GUID, NO_ALLOC);
    PRINTF("Got DB created\n");

    /* DB is in/out */
    *res = input;
    /* and an event for when the results are finished */
    ocrEventCreate(&totallyDoneEvent, OCR_EVENT_ONCE_T, TRUE);
    ocrAddDependence(totallyDoneEvent, absFinalEdt, 0, DB_DEFAULT_MODE);

    /* create the EDT with the done_event as the argument */
    {
        u64 paramv = (u64)totallyDoneEvent;
        ocrGuid_t depv = fibArg;

        ocrGuid_t templateGuid;
        ocrEdtTemplateCreate(&templateGuid, fibEdt, 1, 1);
        ocrEdtCreate(&fibC, templateGuid, 1, &paramv, 1, &depv, EDT_PROP_NONE,
                     NULL_GUID, NULL);
        ocrEdtTemplateDestroy(templateGuid);
    }

    return NULL_GUID;
}
