#include <pthread.h>

struct thread_data {
    int *data_array;
    int *temp_array;
    int my_start_index;
    int my_end_index;
    int my_id;
    int array_size;
    int num_threads;
    pthread_barrier_t *the_barrier;
};

void *parallel_mergesort_worker(void *arg);
int binary_search(int value, int *data_array, int lower_bound, int upper_bound);
