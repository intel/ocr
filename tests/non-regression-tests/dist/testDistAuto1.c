#include <ocr.h>

#define N 4

/**
 * DESC: OCR-DIST - Create EDTs and add-dependence NULL_GUID to satisfy the sink EDT
 */


/* Depends on all the parallel_finishes to complete.  Shuts down the runtime. */
ocrGuid_t done(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrShutdown();
    return NULL_GUID;
}

/* Satisfies one of done()'s input dependencies */
ocrGuid_t parallel_finish(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrGuid_t done_edt = paramv[0];
    int       i        = paramv[1];
    ocrAddDependence(NULL_GUID, done_edt, i, DB_MODE_RO);
    return NULL_GUID;
}

/* Spawns an instance of parallel_finish() from the provided template.
 * NOTE: this often crashes because the template GUID was created on another node. */
ocrGuid_t parallel_spawn(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrGuid_t done_edt        = paramv[0];
    ocrGuid_t finish_template = paramv[1];
    int       i               = paramv[2];
    ocrGuid_t finish_edt;
    u64 args[] = { done_edt, i };
    ocrEdtCreate(&finish_edt, finish_template, 2, args, 0, NULL, EDT_PROP_NONE, NULL_GUID, NULL);
    return NULL_GUID;
}

/* Starts some parallel_spawn() instances, passing a parallel_finish() template in paramv[] */
ocrGuid_t mainEdt(u32 paramc, u64 *paramv, u32 depc, ocrEdtDep_t *depv) {
    ocrGuid_t done_template, finish_template, spawn_template;
    ocrGuid_t done_edt, spawn_edt;
    int i;
    ocrEdtTemplateCreate(&done_template  , done           , 0, N);
    ocrEdtTemplateCreate(&finish_template, parallel_finish, 2, 0);
    ocrEdtTemplateCreate(&spawn_template , parallel_spawn , 3, 0);
    ocrEdtCreate(&done_edt, done_template, 0, NULL, N, NULL, EDT_PROP_NONE, NULL_GUID, NULL);
    for(i = 0; i < N; i++) {
        u64 args[] = { done_edt, finish_template, i };
        ocrEdtCreate(&spawn_edt, spawn_template, 3, args, 0, NULL, EDT_PROP_NONE, NULL_GUID, NULL);
    }
    return NULL_GUID;
}
