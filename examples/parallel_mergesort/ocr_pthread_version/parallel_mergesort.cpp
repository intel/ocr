#include "sequential_mergesort.h"
#include "parallel_mergesort.h"
#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "ocr.h"

#define LOG_NUM_THREADS 3
#define NUM_THREADS 8

pthread_t threads[NUM_THREADS];

int main(int argc, char ** argv){
  
  /* Init OCR */
  OCR_INIT(&argc, argv, parallel_mergesort_worker);

  ocrGuid_t *thread_guids, *event_guids;
  int *sort_array, i;
  ocrGuid_t **event_db_guids, **event_db_addresses;;

  sort_array = new int[ARRAY_SIZE];
  thread_guids = new ocrGuid_t[NUM_THREADS];
  event_guids = new ocrGuid_t[NUM_THREADS];
  event_db_guids = new ocrGuid_t *[NUM_THREADS];
  event_db_addresses = new ocrGuid_t *[NUM_THREADS];

  for (i = 0; i < ARRAY_SIZE; i++){
    sort_array[i] = rand() %1000;
  }

  int rc;
  thread_data **td;

  td = new thread_data *[NUM_THREADS];

  pthread_barrier_t barr;
  if(pthread_barrier_init(&barr, NULL, NUM_THREADS)){
    printf("Couldn't create barrier.\n");
  } 

  for(int t = 1; t< NUM_THREADS; t++){

    td[t] = new(struct thread_data);
    td[t]->data_array = sort_array;
    td[t]->my_id = t;
    td[t]->my_start_index = (ARRAY_SIZE/NUM_THREADS) *t;
    td[t]->my_end_index = ((ARRAY_SIZE/NUM_THREADS) * (t+1)) -1;
    td[t]->the_barrier = &barr;

    /* create the EDT for the worker */
    ocrEdtCreate(&(thread_guids[t]), parallel_mergesort_worker, 1, NULL, (void **) &(td[t]), 0, 1, NULL);
    
    /* create the input event for the worker */
    ocrEventCreate(&(event_guids[t]), OCR_EVENT_STICKY_T, true);

    /* and make a data_block for the event */
    //    ocrDbCreate(event_db_guids[i], (void **) &event_db_addresses[i], sizeof(ocrGuid_t), 0, NULL, NO_ALLOC);

    /* Link the input event to the worker */
    ocrAddDependency(event_guids[t], thread_guids[t], i);

    /* Schedule the worker */
    ocrEdtSchedule(thread_guids[t]);

    /* and satisfy the input event to start the worker */
    ocrEventSatisfy(event_guids[t], NULL);
  }
  
  td[0] = new(struct thread_data);
  td[0]->data_array = sort_array;
  td[0]->my_id = 0;
  td[0]->my_start_index = 0;
  td[0]->my_end_index = (ARRAY_SIZE/NUM_THREADS) -1;
  td[0]->the_barrier = &barr;
  
  (*parallel_mergesort_worker)(1, NULL, (void **) &(td[0]), 1, NULL);
  					
}


u8 parallel_mergesort_worker(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]){
    thread_data *td = (struct thread_data *) paramv[0];
    int *temp_array = new int[ARRAY_SIZE];
    //    std::cout << "starting worker " << td->my_id << "\n";
    /* First, sequentially sort our region of the array */ 
    sequential_mergesort(td->data_array, td->my_start_index, td->my_end_index);
    
    //   std::cout << "Thread " << td->my_id << " entering barrier\n";
    pthread_barrier_wait(td->the_barrier);

    //    std::cout << "Thread " << td->my_id << " starting merge loop\n";

    for (int i = 1; i <= LOG_NUM_THREADS; i++){
      if (!(td->my_id & ((1<<i)-1))){ 
	/* I'm responsible for part of the merge */
	
	
	int merge_start = td->my_start_index;
	int merge_end = (merge_start + (ARRAY_SIZE/NUM_THREADS) * (1 << i)) -1;
	int midpoint = merge_start + (ARRAY_SIZE/NUM_THREADS) * (1 << (i-1));
	//	std::cout << " Thread " << td->my_id << " is a worker on merge loop iteration " << i <<". Merge_start = " << merge_start << ", midpoint = " << midpoint << ", merge_end = " << merge_end << " \n";

	/* Copy the part of the sort array that might get clobbered into the temp array */ 
	for (int i = merge_start; i < midpoint; i++){
	  temp_array[i] = td->data_array[i];
	}

	/* Now, merge */
	int k = merge_start;
	int l = merge_start;
	int j = midpoint;

	while((l< midpoint) && (j <= merge_end)){
	  if (temp_array[l] < td->data_array[j]){
	    td->data_array[k] = temp_array[l];
	    l++;
	  }
	  else{
	    td->data_array[k] = td->data_array[j];
	    j++;
	    /* This can't clobber un-merged data because a sorted item from
	 the second merged array can never move back during the
	 merge. */
	  }
	  k++;
	}
	
	/* We've merged at least half of the array, now just merge in the
	   remaining items from the half that didn't get emptied. */
	/* Either there are items left in temp_array that need to be merged,
	   or the items in the second sub-array of td->data_array were in the right
	   place, and we can leave. */
	if (l < midpoint){
	  while(k <= merge_end){
	    td->data_array[k] = temp_array[l];
	    l++;
	    k++;
	  }
	}	
      }
      
      
      pthread_barrier_wait(td->the_barrier);
    }
    if(td->my_id !=0){
      pthread_exit(NULL);
    }
    else{
      /* check the result, then exit */
      int bad_sort = 0;
      for(int i = 1; i < ARRAY_SIZE; i++){
	if(td->data_array[i-1] > td->data_array[i]){
	  std::cout << "Mis-sorted value found at positions " << i-1 << " and " << i << "\n";
	  bad_sort = 1;
	}
      }
      if (bad_sort == 0){
	std::cout << "Sorted array checks out\n";
      }
      pthread_exit(NULL);
    }
    return 0;
  }
