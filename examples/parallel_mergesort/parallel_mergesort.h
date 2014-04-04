#include <pthread.h>
#include "ocr.h"

struct thread_data {
    int *data_array;
    int my_start_index;
    int my_end_index;
    int my_id;
    pthread_barrier_t *the_barrier;
};

u8 parallel_mergesort_worker(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]);

u8 splitter(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]);

u8 merger(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]);

u8 merge_phi(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]);

u8 mergelet(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]);

int binary_search(int value, int *data_array, int lower_bound, int upper_bound);
