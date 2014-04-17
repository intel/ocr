#include "sequential_mergesort.h"
#include "parallel_mergesort.h"
#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>


int main(int argc, char ** argv){
  pthread_t *threads;
  int *sort_array, *temp_array, i, array_size, num_threads;

   if (argc != 3){
    std::cerr << "Usage: " << argv[0] << " <array_size> <number of threads>\n";
    exit(0);
      }
  else {
    array_size = atoi(argv[1]);
    num_threads = atoi(argv[2]);
   }

   std::cout << "array size: " << array_size << " threads: " << num_threads << " run time (microseconds): ";
   
  sort_array = new int[array_size];
  temp_array = new int[array_size];
  threads = new pthread_t[num_threads];
  for (i = 0; i < array_size; i++){
    sort_array[i] = rand() %1000;
  }
  
 /* Getting into the actual sorter, so compute the start time */
  struct timeval st;
  long int start_time;
  gettimeofday(&st, NULL);
  start_time = (st.tv_sec * 1000000) + st.tv_usec; 

  int rc;
  thread_data *td;

  pthread_barrier_t barr;
  if(pthread_barrier_init(&barr, NULL, num_threads)){
    printf("Couldn't create barrier.\n");
  } 

  for(int t = 1; t< num_threads; t++){

    td = new(struct thread_data);
    td->data_array = sort_array;
    td->temp_array = temp_array;
    td->my_id = t;
    td->my_start_index = (array_size/num_threads) *t;
    td->my_end_index = ((array_size/num_threads) * (t+1)) -1;
    td->array_size = array_size;
    td->num_threads = num_threads;
    td->the_barrier = &barr;
    rc = pthread_create(&threads[t], NULL, parallel_mergesort_worker, td);

    if (rc){
      std::cerr << "Error when creating child thread " << t << "\n";
    }
  }
  
  td = new(struct thread_data);
  td->data_array = sort_array;
  td->temp_array = temp_array;
  td->my_id = 0;
  td->my_start_index = 0;
  td->my_end_index = (array_size/num_threads) -1;
  td->the_barrier = &barr;
  td->array_size = array_size;
  td->num_threads = num_threads;
  (*parallel_mergesort_worker)(td);
					
  struct timeval et;
  gettimeofday(&et, NULL);
  long int end_time = (et.tv_sec * 1000000) + et.tv_usec;
  
  std::cout << (end_time - start_time) << "\n";
  /* for (i = 1; i < array_size; i++){
    std::cout << sort_array[i] << " ";
  }
  std::cout << "\n";
  */
			     
  /* Check the result */
  
  int bad_sort = 0;
  for(i = 1; i < array_size; i++){
    if(sort_array[i-1] > sort_array[i]){
      std::cout << "Mis-sorted value found at positions " << i-1 << " and " << i << "\n";
      bad_sort = 1;
    }
  }
  if (bad_sort == 0){
    std::cout << "Sorted array checks out\n";
    }

   pthread_exit(NULL);
}


  void *parallel_mergesort_worker(void *arg){
    thread_data *td = (struct thread_data *) arg;
    int array_size;
    int num_threads;
    array_size = td->array_size;
    num_threads = td->num_threads;
    int *temp_array = td->temp_array;
    //    std::cout << "starting worker " << td->my_id << " with array size " << array_size <<"\n";
    /* First, sequentially sort our region of the array */ 
    sequential_mergesort(td->data_array, temp_array, td->my_start_index, td->my_end_index);
    
    //   std::cout << "Thread " << td->my_id << " entering barrier\n";
    pthread_barrier_wait(td->the_barrier);

    //    std::cout << "Thread " << td->my_id << " starting merge loop\n";

    // parallel merge of the sorted sub-arrays
    for (int i = 2; i <= num_threads; i = i << 1){
      if (!(td->my_id & (i-1))){ 
	//I'm a leader on this iteration of the merge
	
	int merge_start = td->my_start_index;
	int merge_end = (merge_start + (array_size/num_threads) * i) -1;
	int midpoint = merge_start + (array_size/num_threads) * (i >>1);
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
      /* Original thread will exit itself after checking results */
      pthread_exit(NULL);
    }
    delete[] temp_array;  // free up some memory
  }
