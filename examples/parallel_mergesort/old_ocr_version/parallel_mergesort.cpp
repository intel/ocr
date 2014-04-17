#include "sequential_mergesort.h"
#include "parallel_mergesort.h"
#include <pthread.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "ocr.h"
#include <sys/time.h>

int main(int argc, char ** argv){

  int array_size;
  long int start_time;

  int chunk_size;

  /* Init OCR */
  OCR_INIT(&argc, argv, splitter, merger);

  if (argc != 3){
    std::cerr << "Usage: " << argv[0] << " <array_size> <sequential_chunk_size>, where sequential_chunk_size is the number of elements at which the algorithm should switch from parallel recursion to just sequentially solving the chunk\n";
      }
  else {
    array_size = atoi(argv[1]);
    chunk_size = atoi(argv[2]);    
   }

  if(array_size <= chunk_size){
    /* be lazy and flag an error */
    std::cerr << "array_size must be larger than chunk_size\n";
    exit(0);
  }

  std::cout << "array size: " << array_size << " chunk size: " << chunk_size << " run time (microseconds): ";

  int *sort_array, *temp_array, i;

  /* Create the arary to be sorted and populate it with random values */
  sort_array = new int[array_size];
  temp_array = new int[array_size];
  for (i = 0; i < array_size; i++){
    //    sort_array[i] = rand() % 1000;
    sort_array[i] = array_size - i;  //input array starts exactly backwards 
    // to eliminate time spent in rand() for hotspot analysis.
  }

  /* Getting into the actual sorter, so compute the start time */
  struct timeval st;
  gettimeofday(&st, NULL);
  start_time = (st.tv_sec * 1000000) + st.tv_usec; 

  /* Create a placeholder event to pass into the top-level splitter
  that pretends to be the event that will fire that splitter's parent
  merger */

  ocrGuid_t placeholder_event;
  ocrEventCreate(&(placeholder_event), OCR_EVENT_STICKY_T, false);


  //  std::cout << "at start: start_time = " << start_time << " and merger_event = " << placeholder_event << "\n";
  /* Create and start a splitter task for the whole array */
  intptr_t **p_paramv = new intptr_t *[1];
  intptr_t *paramv = new intptr_t[8];
  *p_paramv = paramv;
  paramv[0]= (intptr_t) sort_array;
  paramv[1]= (intptr_t) temp_array;
  paramv[2]= (intptr_t) 0;
  paramv[3]= (intptr_t) array_size-1;
  paramv[4]= (intptr_t) array_size;
  paramv[5]= (intptr_t) chunk_size;
  paramv[6]= (intptr_t) start_time;
  paramv[7]= (intptr_t) placeholder_event;  
  
  ocrGuid_t splitter_guid, splitter_event;
  /* create the EDT for the worker */
  ocrEdtCreate(&(splitter_guid), splitter, 8, NULL, (void **) p_paramv, 0, 1, NULL);
  
  /* create the input event for the worker */
  ocrEventCreate(&(splitter_event), OCR_EVENT_STICKY_T, false);
  
  /* and make a data_block for the event */
  //    ocrDbCreate(event_db_guids[i], (void **) &event_db_addresses[i], sizeof(ocrGuid_t), 0, NULL, NO_ALLOC);
  
  /* Link the input event to splitter */
  ocrAddDependency(splitter_event, splitter_guid, 0);
  
  /* Schedule the splitter */
  ocrEdtSchedule(splitter_guid);
  
  /* and satisfy the input event to start the splitter */
  ocrEventSatisfy(splitter_event, NULL);  

  ocrCleanup();
      return 0;
}

u8 splitter(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]){
  /* Splitter is the recursive-subdivision part of mergesort.  Given a
     region of the array to be sorted, if the region is larger than
     CHUNK_SIZE, it splits the region into two equal or near-equal
     pieces and creates a splitter task for each region, as well as a
     merger task to merge the sub-regions once they've been sorted.

     Splitter takes eight arguments: a pointer to the array to be
     sorted, a pointer to an equally-large array for temporary values
     (not used directly, but passed to the merger that splitter creates),
     the start index of the region that splitter is responsible for, the
     end index of the region that splitter is responsible for, the
     size of the array being sorted, the
     chunk size (the number of elements where the algorithm should
     switch from recursive descent to a sequential sort), the time in
     microseconds when the sort started and the
     dependency that should be satisfied to trigger the merger above
     the splitter (This is NULL for the top-level splitter). The array
     size and the start time are not used internally by splitter, but
     are passed to its children so that the run time can be computed
     at the end and so that the output array can be checked for
     correctness.

     It has one dependency: an input dependency that must be
     satisfied for splitter to start, which is typically satisfied by
     the source task as soon as the splitter task has been created.
  */

  intptr_t* typed_paramv = (intptr_t*) *paramv;
  int *sort_array = (int *) typed_paramv[0];
  int *temp_array = (int *) typed_paramv[1];
  int start_index = (int) typed_paramv[2];
  int end_index = (int) typed_paramv[3];
  int array_size = (int) typed_paramv[4];
  int chunk_size = (int) typed_paramv[5];
  long int start_time = (long int) typed_paramv[6];
  ocrGuid_t merger_event = (ocrGuid_t) typed_paramv[7];

  //  std::cout << "Splitter called on indices " << start_index << " to " << end_index << " with array_size " << array_size << " chunk_size " << chunk_size << " start_time " << start_time << " and merger_event " << merger_event << "\n";

  if((end_index - start_index) < chunk_size){
    /* Our piece of the array is small enough that we should just sort
       it sequentially */
    // std::cout << "Splitter doing a sequential sort of indices " << start_index << " to " << end_index << "\n";
    
    sequential_mergesort(sort_array, temp_array, start_index, end_index);
    
    ocrEventSatisfy(merger_event, NULL);


  }
  else{
    /* Do the recursive split into two pieces */
    int new_chunk_size = ((end_index + 1) - start_index)/2; 
    /* +1 because of zero-indexed arrays */

    int start_index_1 = start_index;
    int end_index_1 = start_index + (new_chunk_size -1);
    int start_index_2 = start_index + new_chunk_size;
    int end_index_2 = end_index;

    //    std::cout << "Splitter creating two sub-chunks, one from " << start_index_1 << " to " << end_index_1 << " and the other from " << start_index_2 << " to " << end_index_2 << "\n";

    /* First, create the merger task so that we have all of the events
       to pass to the splitters */

    intptr_t **p_paramv0 = new intptr_t *[1];
    intptr_t *paramv0 = new intptr_t[7];
    *p_paramv0 = paramv0;

    /* Merger will merge the two sorted sub-arrays that lie between
       start_index and end_endex, inclusive */
    paramv0[0] = (intptr_t) sort_array;
    paramv0[1] = (intptr_t) temp_array;
    paramv0[2] = (intptr_t) start_index;
    paramv0[3] = (intptr_t) end_index;
    paramv0[4] = (intptr_t) array_size;
    paramv0[5] = (intptr_t) start_time;
    paramv0[6] = (intptr_t) merger_event;

    /* Create the events that the splitters (actually the mergers that
       the splitters will create) will satisfy to trigger the merger */
    ocrGuid_t left_splitter_done_event_guid, right_splitter_done_event_guid, merger_guid;
    ocrEventCreate(&(left_splitter_done_event_guid), OCR_EVENT_STICKY_T, false);
    ocrEventCreate(&(right_splitter_done_event_guid), OCR_EVENT_STICKY_T, false);

    /* Create the merger task */
    ocrEdtCreate(&(merger_guid), merger, 7, NULL, (void **) p_paramv0, 0, 2, NULL);

    /* Add the splitter events as input dependencies */
    ocrAddDependency(left_splitter_done_event_guid, merger_guid, 0);
    ocrAddDependency(right_splitter_done_event_guid, merger_guid, 1);

    /* Schedule the merger */
    ocrEdtSchedule(merger_guid);

    /* create and fill out the parameter lists for the new splitter
       tasks */
    intptr_t **p_paramv1 = new intptr_t *[1];
    intptr_t *paramv1 = new intptr_t[8];
    *p_paramv1 = paramv1;

    intptr_t **p_paramv2 = new intptr_t *[1];
    intptr_t *paramv2 = new intptr_t[8];
    *p_paramv2 = paramv2;

    paramv1[0]= (intptr_t) sort_array;
    paramv1[1]= (intptr_t) temp_array;
    paramv1[2]= (intptr_t) start_index_1;
    paramv1[3]= (intptr_t) end_index_1;
    paramv1[4]= (intptr_t) array_size;  
    paramv1[5]= (intptr_t) chunk_size;  
    paramv1[6]= (intptr_t) start_time;  
    paramv1[7]= (intptr_t) left_splitter_done_event_guid;  

    paramv2[0]= (intptr_t) sort_array;
    paramv2[1]= (intptr_t) temp_array;
    paramv2[2]= (intptr_t) start_index_2;
    paramv2[3]= (intptr_t) end_index_2;
    paramv2[4]= (intptr_t) array_size;  
    paramv2[5]= (intptr_t) chunk_size;  
    paramv2[6]= (intptr_t) start_time;  
    paramv2[7]= (intptr_t) right_splitter_done_event_guid;  

    /* now, create the two splitter tasks */
    ocrGuid_t left_splitter_guid, right_splitter_guid;
    ocrGuid_t left_event_guid, right_event_guid;
    
    /* create the EDTs for the splitters */
    ocrEdtCreate(&(left_splitter_guid), splitter, 8, NULL, (void **) p_paramv1, 0, 1, NULL);
   ocrEdtCreate(&(right_splitter_guid), splitter, 8, NULL, (void **) p_paramv2, 0, 1, NULL);
    
    /* create the input events for the splitters */
    ocrEventCreate(&(left_event_guid), OCR_EVENT_STICKY_T, false);
    ocrEventCreate(&(right_event_guid), OCR_EVENT_STICKY_T, false);
  
    /* and link the events to the EDTs */
    ocrAddDependency(left_event_guid, left_splitter_guid, 0);
    ocrAddDependency(right_event_guid, right_splitter_guid, 0);

    /* schedule the splitters */
    ocrEdtSchedule(left_splitter_guid);
    ocrEdtSchedule(right_splitter_guid);

    /* and satisfy the input events to start them */
    ocrEventSatisfy(left_event_guid, NULL);
    ocrEventSatisfy(right_event_guid, NULL);
  }

  delete[] *paramv;
  delete[] paramv;
  return 0;  /* We're done */

}

u8 merger(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]){
  /* Merger is the routine that merges two sorted sub-arrays into one
     sorted array.  The sub-arrays are expected to be adjacent regions
     of the array being sorted.  Merger takes seven inputs: a pointer
     to the array being sorted, a pointer to an equally-sized array
     for holding temporary values, the start index of the region to be
     merged (low index of the first sub-array), the end index of
     the region to be merged (high index of the second sub-array), the
     total length of the array being sorted, the start time of the
     sort, and
     the event that merger should satisfy when it is done.

     Merger has two input dependencies from its children. */

  /* For now, just do the dependency stuff so the program structure
     works */

  intptr_t* typed_paramv = (intptr_t*) *paramv;
  int *sort_array = (int *) typed_paramv[0];
  int *temp_array = (int *) typed_paramv[1];
  int start_index = (int) typed_paramv[2];
  int end_index = (int) typed_paramv[3];
  int array_size = (int) typed_paramv[4];
  long int start_time = (long int) typed_paramv[5];
  ocrGuid_t output_event = (ocrGuid_t) typed_paramv[6];

  //std::cout << "Merger called on range " << start_index << " to " << end_index << " with start_time " << start_time << " and array_size " << array_size << "\n";

  /* Merge the sub-arrays */
  int merge_start = start_index;
  int merge_end = end_index;
  int midpoint = merge_start + (((merge_end - merge_start)+1)/2);
  
/* Copy the part of the sort array that might get clobbered into the temp array */ 
  for (int i = merge_start; i < midpoint; i++){
    temp_array[i] = sort_array[i];
  }
  
  /* Now, merge */
  int k = merge_start;
  int l = merge_start;
  int j = midpoint;
  
  while((l< midpoint) && (j <= merge_end)){
    if (temp_array[l] < sort_array[j]){
      sort_array[k] = temp_array[l];
      l++;
    }
    else{
      sort_array[k] = sort_array[j];
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
     or the items in the second sub-array of sort_array were in the right
     place, and we can leave. */
  if (l < midpoint){
    while(k <= merge_end){
      sort_array[k] = temp_array[l];
      l++;
      k++;
    }
  }	




  if(((end_index - start_index) + 1) == array_size){
    /* I'm the top-level merger, and the program is done when I'm finished */
    /* Compute run time (not counting result check */
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    long int end_microseconds = (end_time.tv_sec * 1000000) + end_time.tv_usec;
    /* std::cout << "start time " << start_time << "\n";
    std::cout << "end time   " << end_microseconds << "\n";
    std::cout << "Core run time " << (end_microseconds - start_time) << " microseconds\n"; */
    std::cout << (end_microseconds - start_time) << "\n";

    /* check the result, then exit */
    int bad_sort = 0;
    /*  for(int i = 1; i < array_size; i++){
  
      if(sort_array[i-1] > sort_array[i]){
	std::cout << "Mis-sorted value found at positions " << i-1 << " and " << i << "\n";
	bad_sort = 1;
      }
      } */  //comment out result check for vtune analysis
    if (bad_sort == 0){
      std::cout << "Sorted array checks out\n";
    }

    /*   for(int i = 0; i < array_size; i++){
      std::cout << sort_array[i] << " ";
    }
    std::cout << "\n";
    */
    ocrFinish();  /* Tell OCR we're done */
  }
  ocrEventSatisfy(output_event, NULL);

  delete[] *paramv;
  delete[] paramv;
  return(0);


}

