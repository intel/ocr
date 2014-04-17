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
  // std::cout << "At start, sort_array = " << sort_array << " and  temp_array " << temp_array <<  "\n";
  threads = new pthread_t[num_threads];
  for (i = 0; i < array_size; i++){
    sort_array[i] = rand() %1000000000; 
    //  std::cout << sort_array[i] << " ";
  }
  //  std::cout << "\n";

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
    int *data_array = td->data_array;
//    std::cout << "starting worker " << td->my_id << " with array size " << array_size <<"\n";
    /* First, sequentially sort our region of the array */ 
    sequential_mergesort(data_array, temp_array, td->my_start_index, td->my_end_index);
    
    //   std::cout << "Thread " << td->my_id << " entering barrier\n";
    pthread_barrier_wait(td->the_barrier);

    //    std::cout << "Thread " << td->my_id << " starting merge loop\n";

    // parallel merge of the sorted sub-arrays

    int thread_subarray_chunk_size = array_size / ( 2 * num_threads);
    // amount of each of the two subarrays being merged that a thread processes
    int num_iterations = 0;
    for (int i = 2; i <= num_threads; i = i << 1){
      // i = number of threads involved in this merge 
      
      // the set of threads involved in a merge ranges from ids 0 mod i 
      // to i-1 mod i
      num_iterations++; //use this to determine if we need to do one
			//extra copy at the end.

      int lowest_partner_thread = td->my_id & ~(i-1);

      int subarray1_start = lowest_partner_thread * (array_size/num_threads);
      int subarray2_start = subarray1_start + (i * thread_subarray_chunk_size);
      int subarray2_end = subarray2_start + (i * thread_subarray_chunk_size) -1;

      int my_subarray1_start = subarray1_start + ((td->my_id % i) * thread_subarray_chunk_size);   
      // Start of the part of subarray1 that I will merge
      int next_subarray1_start = my_subarray1_start + thread_subarray_chunk_size; 
      //start of the part of subarray1 that the next thread will merge.
      
      int my_subarray2_start ;
      
      if(my_subarray1_start != subarray1_start){
	my_subarray2_start = binary_search(data_array[my_subarray1_start], data_array, subarray2_start, subarray2_end); 
	// Find the position in the second subarray of the element that 
	// corresponds to the start of our region of the first subarray
      }
      else{
	my_subarray2_start = subarray2_start;
	// the low thread of the merge always starts merging at the
	// beginning of subarray2
      }
      int my_subarray2_end;
      
      if (next_subarray1_start < subarray2_start){
	 my_subarray2_end = binary_search(data_array[next_subarray1_start], data_array, subarray2_start, subarray2_end) -1;
	// Find the position in the second subarray that corresponds to
	// the start of the next thread's region of the first subarray,
	// and then back up one.
      }
      else{
	// Handle the case where subarray2 is the end of the data array
	my_subarray2_end = subarray2_end;
      }
     
      /* Copy the part of the sort array that might get clobbered into the temp array */ 
      
      int subarray1_index = my_subarray1_start;
      int subarray2_index = my_subarray2_start;
      int merged_array_index = (my_subarray1_start - subarray1_start) + (my_subarray2_start - subarray2_start) + subarray1_start; 
      // location where the first of our merged arrays should go

      //   std::cout << "Thread " << td->my_id << " merging subarray1 from " << my_subarray1_start << " to " << next_subarray1_start -1 << " with subarray2 from " << my_subarray2_start << " to " << my_subarray2_end << " and putting results in temp_array starting at " << merged_array_index <<"\n";

      while((subarray1_index < next_subarray1_start) && (subarray2_index <= my_subarray2_end)){
	if(data_array[subarray1_index] < data_array[subarray2_index]){
	  temp_array[merged_array_index] = data_array[subarray1_index];
	  // std::cout << "Thread " << td->my_id << " writing value " << data_array[subarray1_index] << " to element " << merged_array_index << " of temp_array\n";
	  subarray1_index++;
	}
	else{
	  temp_array[merged_array_index] = data_array[subarray2_index];
	  //  std::cout << "Thread " << td->my_id << " writing value " << data_array[subarray2_index] << " to element " << merged_array_index << " of temp_array\n";
	  subarray2_index++;
	}
	merged_array_index++;
      }
     
      // At this point, we've merged all of one of the two sub-arrays
      // with at least some of the other, so just copy the remaining
      // elements of the other array into place

      if(subarray1_index < next_subarray1_start){
	// there were some elements of subarray1 left
	while(subarray1_index < next_subarray1_start){
	  temp_array[merged_array_index] = data_array[subarray1_index];
	  //	 	  std::cout << "Thread " << td->my_id << " writing value " << data_array[subarray1_index] << " from subarray1 to element " << merged_array_index << " of temp_array\n";
	  subarray1_index++;
	  merged_array_index++;
	}
      }
      else{
	while(subarray2_index <= my_subarray2_end){
	  temp_array[merged_array_index] = data_array[subarray2_index];
	  //  std::cout << "Thread " << td->my_id << " writing value "
	  // << data_array[subarray2_index] << " from subarray 2 to element " << merged_array_index << " of temp_array\n";
	  subarray2_index++;
	  merged_array_index++;
	}  
      }
      // Exchange data array and temp array pointers 
      int *temp = data_array;
      data_array = temp_array;
      temp_array = temp;
      // Ok, done with this iteration of the merge, barrier and go on
      // to the next
      pthread_barrier_wait(td->the_barrier);
      
    }
    if(num_iterations % 2){
      //      std::cout << "thread " << td->my_id << " doing copy back from " << data_array << " to " << temp_array << "\n";
      /* We've shuffled arrays an odd number of times, need to copy
	 back once more */
      
      for(int i = td->my_id * (array_size/num_threads); i < ((td->my_id + 1) * (array_size/num_threads)); i++){
	temp_array[i] = data_array[i];
      }
      pthread_barrier_wait(td->the_barrier);
    }
    
    if(td->my_id !=0){
      /* Original thread will exit itself after checking results */
      pthread_exit(NULL);
    }
  }

int binary_search(int value, int *data_array, int lower_bound, int upper_bound){
  /* Standard binary search algorithm */
  int start = lower_bound;
  int end = upper_bound;

  while(start <= end){
    int midpoint = (start + end) / 2;
    if(data_array[midpoint] == value){
      return(midpoint);
    }
    if(data_array[midpoint] < value){
      // we guessed too low
      start = midpoint + 1;
    }
    else{
      // we guessed too high
      end = midpoint -1;
    }
  }

  // if we got this far, the value wasn't in the array, so return the
  // best approximation to its position
  if(end < 0){
    return 0;
  }
  if(start > upper_bound){
    return upper_bound;
  }
  else{
    return start;
  }

}
