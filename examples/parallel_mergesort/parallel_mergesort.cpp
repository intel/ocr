#include "sequential_mergesort.h"
#include "parallel_mergesort.h"
#include <pthread.h>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "ocr.h"
#include <sys/time.h>

long int start_time; /* Make this a global rather than passing it
			around like a crazy person */

int main(int argc, char ** argv){

  int array_size;
  extern long int start_time;

  int chunk_size;

  /* Init OCR */
  OCR_INIT(&argc, argv, splitter, merger, merge_phi, mergelet);

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

  
  /* Approximate the base-two log of the chunk_size */
  int log_chunk_size = 0;
  for(unsigned int temp = array_size; temp > 0; temp = temp >> 1){
    log_chunk_size += 1;
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

  //ocrGuid_t placeholder_event;
  //ocrEventCreate(&(placeholder_event), OCR_EVENT_STICKY_T, false);


  //  std::cout << "at start: start_time = " << start_time << " and merger_event = " << placeholder_event << "\n";
  /* Create and start a splitter task for the whole array */
  ocrGuid_t splitterInputGuid, splitterResultGuid;
  intptr_t *splitterInputDb, *splitterResultDb;
   
  if(int res = ocrDbCreate(&splitterInputGuid, (void **) &splitterInputDb, 8*sizeof(intptr_t), 0, NULL, NO_ALLOC)){
    std::cout << "DbCreate #1 failed\n";
  }
  /*  else{
    std::cout << "DbCreate #1 allocated db for guid " << splitterInputGuid << "\n";
    }*/

  splitterInputDb[0]= (intptr_t) sort_array;
  splitterInputDb[1]= (intptr_t) temp_array;
  splitterInputDb[2]= (intptr_t) 0;
  splitterInputDb[3]= (intptr_t) array_size-1;
  splitterInputDb[4]= (intptr_t) array_size;
  splitterInputDb[5]= (intptr_t) chunk_size;
  splitterInputDb[6] = (intptr_t) log_chunk_size;
  splitterInputDb[7]= (intptr_t) NULL_GUID;  
  
  ocrGuid_t splitter_guid, splitter_event, splitter_event2;
  /* create the EDT for the worker */
  ocrEdtCreate(&(splitter_guid), splitter, 0, NULL, NULL, 0, 1, NULL);
  
  /* create the event for passing the inputs */
  ocrEventCreate(&(splitter_event), OCR_EVENT_STICKY_T, true);
   
  /* Link the input events to splitter */
  ocrAddDependency(splitter_event, splitter_guid, 0);

  /* Schedule the splitter */
  ocrEdtSchedule(splitter_guid);
  
  /* and satisfy the input events to start the splitter */
  ocrEventSatisfy(splitter_event, splitterInputGuid);  

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

     It has one dependency: an input dependency that passes the
     arguments to splitter.
  */

  intptr_t* typed_paramv = (intptr_t*) depv[0].ptr;
  int *sort_array = (int *) typed_paramv[0];
  int *temp_array = (int *) typed_paramv[1];
  int start_index = (int) typed_paramv[2];
  int end_index = (int) typed_paramv[3];
  int array_size = (int) typed_paramv[4];
  int chunk_size = (int) typed_paramv[5];
  int log_chunk_size = (int) typed_paramv[6];
  ocrGuid_t merger_event = (ocrGuid_t) typed_paramv[7];
  std::ostringstream buf;

  //buf << "Splitter called on indices " << start_index << " to " << end_index << " with sort_array " << sort_array << " and temp_array " << temp_array << "\n";
  
  //std::cout << buf.str();

  if((end_index - start_index) < chunk_size){
    /* Our piece of the array is small enough that we should just sort
       it sequentially */
    // std::cout << "Splitter doing a sequential sort of indices " << start_index << " to " << end_index << "\n";
    
    sequential_mergesort(sort_array, temp_array, start_index, end_index);
    /* Create the array to hold the sorted sub-array */
    ocrGuid_t result_array_guid;
    int *result_array;
    if(int res = ocrDbCreate(&result_array_guid, (void **) &result_array, ((end_index - start_index) +1) *sizeof(int), 0, NULL, NO_ALLOC)){
      std::cout << "DBcreate #2 failed with result " << res << "\n";
    }
    /*  else{
    std::cout << "DbCreate #2 allocated db for guid " <<
    result_array_guid << "\n"; 
    }*/
    int sort_position = start_index;
    /* Copy the sorted sub-array into our result datablock */
    for(int i = 0; i < (end_index - start_index) + 1; i++){
      result_array[i] = sort_array[sort_position];
      sort_position++;
    }

    ocrEventSatisfy(merger_event, result_array_guid);

    //   ocrDbRelease(result_array_guid);
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
   
    /* Create the events that the splitters (actually the mergers that
       the splitters will create) will satisfy to trigger the merger */
    ocrGuid_t left_splitter_done_event_guid, right_splitter_done_event_guid, merger_guid, mergerInputReadyGuid, mergerInputGuid;
    intptr_t *mergerInputDb;

    ocrEventCreate(&(left_splitter_done_event_guid), OCR_EVENT_STICKY_T, true);
    ocrEventCreate(&(right_splitter_done_event_guid), OCR_EVENT_STICKY_T, true);
    ocrEventCreate(&(mergerInputReadyGuid), OCR_EVENT_STICKY_T, true);
    
    /* Create the merger input array */
    if(int res =ocrDbCreate(&mergerInputGuid, (void **) &mergerInputDb, 5*sizeof(intptr_t), 0, NULL, NO_ALLOC)){
      std::cout << "DBcreate #3.5 failed with result " << res << "\n";
    }

    mergerInputDb[0] = (intptr_t) (end_index - start_index) + 1;
    mergerInputDb[1] = (intptr_t) array_size;
    mergerInputDb[2] = (intptr_t) log_chunk_size;
    mergerInputDb[3] = (intptr_t) chunk_size;
    mergerInputDb[4] = (intptr_t) merger_event;

      /* Create the merger task */
    ocrEdtCreate(&(merger_guid), merger, 0, 0, NULL, 0, 3, NULL);

    /* Add the splitter events as input dependencies */
    ocrAddDependency(left_splitter_done_event_guid, merger_guid, 0);
    ocrAddDependency(right_splitter_done_event_guid, merger_guid, 1);
    ocrAddDependency(mergerInputReadyGuid, merger_guid, 2);

    /* Schedule the merger */
    ocrEdtSchedule(merger_guid);

    /* and deliver its input values */
    ocrEventSatisfy(mergerInputReadyGuid, mergerInputGuid);

    /* create and fill out the parameter lists for the new splitter
       tasks */
    ocrGuid_t leftSplitterInputGuid, rightSplitterInputGuid;
    intptr_t *leftSplitterInputDb, *rightSplitterInputDb;
 
    if(int res =ocrDbCreate(&leftSplitterInputGuid, (void **) &leftSplitterInputDb, 8*sizeof(intptr_t), 0, NULL, NO_ALLOC)){
      std::cout << "DBcreate #4 failed with result " << res << "\n";
    }
    /*  else{
    std::cout << "DbCreate #4 allocated db for guid " << leftSplitterInputGuid << "\n";
    }*/
    if(int res =    ocrDbCreate(&rightSplitterInputGuid, (void **) &rightSplitterInputDb, 8*sizeof(intptr_t), 0, NULL, NO_ALLOC)){
      std::cout << "DBcreate #5 failed with result " << res << "\n";
    } 
    /*    else{
      std::cout << "DbCreate #5 allocated db for guid " << rightSplitterInputGuid << "\n";
      } */
    leftSplitterInputDb[0]= (intptr_t) sort_array;
    leftSplitterInputDb[1]= (intptr_t) temp_array;
    leftSplitterInputDb[2]= (intptr_t) start_index_1;
    leftSplitterInputDb[3]= (intptr_t) end_index_1;
    leftSplitterInputDb[4]= (intptr_t) array_size;  
    leftSplitterInputDb[5]= (intptr_t) chunk_size;  
    leftSplitterInputDb[6]= (intptr_t) log_chunk_size;  
    leftSplitterInputDb[7]= (intptr_t) left_splitter_done_event_guid;  

    rightSplitterInputDb[0]= (intptr_t) sort_array;
    rightSplitterInputDb[1]= (intptr_t) temp_array;
    rightSplitterInputDb[2]= (intptr_t) start_index_2;
    rightSplitterInputDb[3]= (intptr_t) end_index_2;
    rightSplitterInputDb[4]= (intptr_t) array_size;  
    rightSplitterInputDb[5]= (intptr_t) chunk_size;  
    rightSplitterInputDb[6]= (intptr_t) log_chunk_size;  
    rightSplitterInputDb[7]= (intptr_t) right_splitter_done_event_guid;  


    /* now, create the two splitter tasks */
    ocrGuid_t left_splitter_guid, right_splitter_guid;
    ocrGuid_t left_event_guid, right_event_guid;
    
    /* create the EDTs for the splitters */
    ocrEdtCreate(&(left_splitter_guid), splitter, 0, NULL, NULL, 0, 1, NULL);
    ocrEdtCreate(&(right_splitter_guid), splitter, 0, NULL, NULL, 0, 1, NULL);
    
    /* create the input events for the splitters */
    ocrEventCreate(&(left_event_guid), OCR_EVENT_STICKY_T, true);
    ocrEventCreate(&(right_event_guid), OCR_EVENT_STICKY_T, true);

  
    /* and link the events to the EDTs */
    ocrAddDependency(left_event_guid, left_splitter_guid, 0);
    ocrAddDependency(right_event_guid, right_splitter_guid, 0);

    /* schedule the splitters */
    ocrEdtSchedule(left_splitter_guid);
    ocrEdtSchedule(right_splitter_guid);

    /* and satisfy the input events to start them */
    ocrEventSatisfy(left_event_guid, leftSplitterInputGuid);
    ocrEventSatisfy(right_event_guid, rightSplitterInputGuid);
   }

  if(int res=ocrDbDestroy(depv[0].guid)){  /* Free the space used by our inputs */
    std::cout << "Splitter ocrDbDestroy #1 failed with code " << res << "\n";
  }
  /*  else{
    std::cout << "Splitter ocrDbDestroy #1 destroyed guid " << depv[0].guid << "\n";
    }*/
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

     Merger has two input dependencies from its children, and one that
     contains its inputs from the task that created it*/

  intptr_t* typed_paramv = (intptr_t *) depv[2].ptr;
  int merge_length = (int) typed_paramv[0];
  u64 array_size = (u64) typed_paramv[1];
  u64 log_chunk_size = (u64) typed_paramv[2];
  u64 chunk_size = (u64) typed_paramv[3];
  ocrGuid_t output_event = (ocrGuid_t) typed_paramv[4];
  
  u64 merge_chunk_size = log_chunk_size * chunk_size; 
  /*std::ostringstream buf;
    buf << "Merger called with merge_length = " << merge_length << "\n";
  std::cerr << buf.str();
  */
  int *left_subarray = (int *) depv[0].ptr;
  int *right_subarray = (int *) depv[1].ptr;
  
  /* Pre-check the subarrays for correctness as a debugging step */
  /* for(int i = 1; i < merge_length/2; i++){
    if(left_subarray[i] < left_subarray[i-i]){
      std::cout << "miss-sorted element found in left subarray at position " << i << "\n";
    }
  }
  for(int i = 1; i < merge_length/2; i++){
    if(right_subarray[i] < right_subarray[i-i]){
      std::cout << "miss-sorted element found in right subarray at position " << i << "\n";
    }
  }
  */
  /* allocate the space for the merged array */
  ocrGuid_t outputArrayGuid;
  int *output_array;
  if(int res =ocrDbCreate(&outputArrayGuid, (void **) &output_array, merge_length*sizeof(int), 0, NULL, NO_ALLOC)){
      std::cout << "DBcreate #6 failed with result " << res << "\n";
    }
  /*  else{
    std::cout << "DbCreate #6 allocated db for guid " << outputArrayGuid << "\n";
    }*/


  if(merge_length > merge_chunk_size){
  
  /* do the merge in parallel */

    int num_mergelets = merge_length/merge_chunk_size;
    if (num_mergelets > 61){
      num_mergelets = 61;
      // keep the number of mergelets from becoming absurdly large for
      // very small chunk sizes and thus blowing out the deque length 
    }
    if (merge_length % merge_chunk_size){
      num_mergelets += 1;
    }  /* add an odd-length mergelet if things don't divide evenly */

    /* create the merge_phi task that acts as a barrier for the
       mergelets */ 

    ocrGuid_t mergePhiGuid;
    ocrEdtCreate(&(mergePhiGuid), merge_phi, 0, (u64 *) output_event, (void **) array_size, 0, (num_mergelets +3), NULL);
    //std::cout << "output_event = " << output_event <<"\n";
    /* add the events for the input arrays */
    ocrGuid_t mergePhiLeftArrayGuid, mergePhiRightArrayGuid, mergePhiOutputArrayGuid;

    ocrEventCreate(&(mergePhiLeftArrayGuid), OCR_EVENT_STICKY_T, true);  
    ocrEventCreate(&(mergePhiRightArrayGuid), OCR_EVENT_STICKY_T, true); 
    ocrEventCreate(&(mergePhiOutputArrayGuid), OCR_EVENT_STICKY_T, true);  

    /* add the dependencies */
    ocrAddDependency(mergePhiLeftArrayGuid, mergePhiGuid, 0);
    ocrAddDependency(mergePhiRightArrayGuid, mergePhiGuid, 1);
    ocrAddDependency(mergePhiOutputArrayGuid, mergePhiGuid, 2);

    /* and satisfy them to queue things up */
    ocrEventSatisfy(mergePhiLeftArrayGuid, depv[0].guid);
    ocrEventSatisfy(mergePhiRightArrayGuid, depv[1].guid);
    ocrEventSatisfy(mergePhiOutputArrayGuid, outputArrayGuid);
    
    /* merge_phi setup mostly done.  Create the mergelets and add
       their dependencies */
    ocrGuid_t mergeletGuid, mergeletLeftInputArrayGuid, mergeletRightInputArrayGuid, mergeletOutputArrayGuid, *mergeletDoneGuid_p;
    intptr_t *paramv, **p_paramv;

    int left_array_start = 0;
    int left_array_length = (merge_length /2) / num_mergelets;
    int left_array_end = -1;
    int right_array_start = 0;
    int right_array_end = 0 ;
    int output_array_start = 0;
    //    std::ostringstream buf;
    for (int i = 0; i < num_mergelets; i++){
      mergeletDoneGuid_p = new ocrGuid_t;
      ocrEventCreate(mergeletDoneGuid_p, OCR_EVENT_STICKY_T, false);
      ocrAddDependency(*mergeletDoneGuid_p, mergePhiGuid, (3+i));
      /* create the "done" event for the mergelet and add it to the
	 dependency list for merge_phi */
      
      /* Compute end points of this mergelet's merge */
      left_array_end += left_array_length;
      if(i == num_mergelets -1){
	left_array_end = (merge_length/2) -1;
	left_array_length = (left_array_end - left_array_start) + 1;
	//deal with rounding errors 
      }

      right_array_end = binary_search(left_subarray[left_array_end], right_subarray, 0, ((merge_length/2) -1));

      //    std::cout << *mergeletDoneGuid_p << " ";
      /* Allocate the input array for the mergelet and initialize it */
      paramv = new intptr_t[6];
      p_paramv = new intptr_t *[1];

      *p_paramv = paramv;
      paramv[0] = (intptr_t) left_array_start;
      paramv[1] = (intptr_t) left_array_length;
      paramv[2] = (intptr_t) right_array_start;
      paramv[3] = (intptr_t) (right_array_end - right_array_start) +1;
      paramv[4] = (intptr_t) output_array_start;
      paramv[5] = (intptr_t) *mergeletDoneGuid_p;
      
      /*buf << "Creating mergelet with left_array_start = " << left_array_start << ", left_array_length = " << left_array_length << ", right_array_start = " << right_array_start << ", and right_array_length= " << (right_array_end - right_array_start)+1 << "\n";

	std::cerr << buf.str();  */
      /* Create the EDT for the mergelet */
      ocrEdtCreate(&(mergeletGuid), mergelet, 6, NULL, (void **) p_paramv, 0, 3, NULL);
      /* Create the events for the input and output arrays for
	 mergelet */
      ocrEventCreate(&(mergeletLeftInputArrayGuid), OCR_EVENT_STICKY_T, true);  
      ocrEventCreate(&(mergeletRightInputArrayGuid), OCR_EVENT_STICKY_T, true);  
      ocrEventCreate(&(mergeletOutputArrayGuid), OCR_EVENT_STICKY_T, true);  

      ocrAddDependency(mergeletLeftInputArrayGuid, mergeletGuid, 0);
      ocrAddDependency(mergeletRightInputArrayGuid, mergeletGuid, 1);
      ocrAddDependency(mergeletOutputArrayGuid, mergeletGuid, 2);

      /* Schedule the mergelet and satisfy its input dependencies so
	 that it starts*/
      ocrEdtSchedule(mergeletGuid);
      ocrEventSatisfy(mergeletLeftInputArrayGuid, depv[0].guid);
      ocrEventSatisfy(mergeletRightInputArrayGuid, depv[1].guid);
      ocrEventSatisfy(mergeletOutputArrayGuid, outputArrayGuid);

      /* Now, bump the start indices for the next loop */
      output_array_start = right_array_end + left_array_end + 2;
      right_array_start = right_array_end +1;
      left_array_start += left_array_length;
      delete mergeletDoneGuid_p;  /* Free this so we don't leak memory */
    }
    /* once all the dependencies are in place, schedule the merge_phi */
    ocrEdtSchedule(mergePhiGuid); 
    //std::cout << "\n";
  }
  else{

    /* Merge the sub-arrays */  
    int left_index = 0;
    int right_index = 0;
    int merged_index = 0;
    
    while((left_index < merge_length/2) && (right_index < merge_length/2)){
      if(left_subarray[left_index] < right_subarray[right_index]){
	output_array[merged_index] = left_subarray[left_index];
      left_index++;
      }
      else{
	output_array[merged_index] = right_subarray[right_index];
	right_index++;
      }
      merged_index++;
    }
    
    // ok, we've reached the end of at least one of the sub-arrays, so finish up
    while(left_index < merge_length/2){
      output_array[merged_index] = left_subarray[left_index];
      left_index++;
      merged_index++;
    }
    while(right_index < merge_length/2){
      output_array[merged_index] = right_subarray[right_index];
      right_index++;
      merged_index++;
    }
    
    //test loop
    /*  for(int q = 1; q < merge_length; q++){
	if (output_array[q-1] > output_array[q]){
	std::cout << "Mis-sorted value found at position " << q << " with start_index = " << start_index << " and end_index " << end_index << "\n";
    }
    }*/
    if(output_event == NULL_GUID){
      /* I'm the top-level merger, and the program is done when I'm finished */
      /* Compute run time (not counting result check */
      extern long int start_time;
      struct timeval end_time;
      gettimeofday(&end_time, NULL);
    long int end_microseconds = (end_time.tv_sec * 1000000) + end_time.tv_usec;
    /* std::cout << "start time " << start_time << "\n";
       std::cout << "end time   " << end_microseconds << "\n";
       std::cout << "Core run time " << (end_microseconds - start_time) << " microseconds\n"; */
    std::cout << (end_microseconds - start_time) << "\n";
    
    /* check the result, then exit */
    int bad_sort = 0;
    for(int i = 1; i < array_size; i++){
  
      if(output_array[i-1] > output_array[i]){
	std::cout << "Mis-sorted value found at positions " << i-1 << " and " << i << "\n";
	bad_sort = 1;
      }
    }   //comment out result check for vtune analysis
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
    else{
      ocrEventSatisfy(output_event, outputArrayGuid);
    }
    if(int res = ocrDbDestroy(depv[1].guid)){  /* Free the space used by our inputs */
      std::cout << "Merger ocrDbDestroy #2 failed with code " << res << "\n";
     }
    /*  else{
	std::cout << "Merger ocrDbDestroy #2 destroyed guid " << depv[1].guid << "\n";
	}*/
    if(int res = ocrDbDestroy(depv[0].guid)){  /* Free the space used by our inputs */
     std::cout << "Merger ocrDbDestroy #3 failed with code " << res << "\n";
    }
    /*  else{
	std::cout << "Merger ocrDbDestroy #1 destroyed guid " << depv[0].guid << "\n";
	}*/
  }
  
  return(0);
}

u8 merge_phi(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]){
  /* merge_phi is the synchronization node for the parallel merge.  It
     takes N+3 dependencies, where N is the number of mergelets fired
     off in parallel by the overall merger.  The first three
     dependencies are the left input array, right input array, and
     output array from the merger that created merge_phi.  The
     remaining ones are completion events from the mergelets.

     When triggered, merge_phi signals its parent's parent's merger
     that it has completed, passing the output array that the
     mergelets have filled in.  It then frees the left and right input
     arrays. */
  //std::cerr << "Merge_phi firing\n";
  ocrGuid_t output_event = (ocrGuid_t) params;
  u64 array_size = (u64) paramv;
  ocrGuid_t outputArrayGuid = depv[2].guid;
  if (output_event == NULL_GUID){
    /* We've reached the top of the tree, and are done */
    //   std::cout << "We're at the top of the tree, checking results\n";

    extern long int start_time;
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    long int end_microseconds = (end_time.tv_sec * 1000000) + end_time.tv_usec;
    /* std::cout << "start time " << start_time << "\n";
       std::cout << "end time   " << end_microseconds << "\n";
       std::cout << "Core run time " << (end_microseconds - start_time) << " microseconds\n"; */
    std::cout << (end_microseconds - start_time) << "\n";
    int *output_array = (int *) depv[2].ptr;

    int bad_sort = 0;
    for(int i = 1; i < array_size; i++){
      //  std::cout << output_array[i] << " ";
      if(output_array[i-1] > output_array[i]){
	std::cout << "Mis-sorted value found at positions " << i-1 << " and " << i << "\n";
	bad_sort = 1;
      }
    }   //comment out result check for vtune analysis
    if (bad_sort == 0){
      std::cout << "Sorted array checks out\n";
    }
    ocrFinish(); // Tell OCR we're done
  }
  else{
    /* We're not at the top of the tree, satisfy our output event to
       let the next level start */ 
    ocrEventSatisfy(output_event, outputArrayGuid);
  }

    if(int res = ocrDbDestroy(depv[0].guid)){  /* Free the space used by our inputs */
    std::cout << "Merge_phi ocrDbDestroy #1 failed with code " << res << "\n";
   }

  if(int res = ocrDbDestroy(depv[1].guid)){  /* Free the space used by our inputs */
      std::cout << "Merge_phi ocrDbDestroy #2 failed with code " << res << "\n";
     }


  return(0);
}

u8 mergelet(u32 paramc, u64 * params, void *paramv[], u32 depc, ocrEdtDep_t depv[]){

  /* Mergelet implements the parallel merge.  It takes three input
     datablocks: its left input array, its right input array, and its
     output array.  It takes six arguments: the starting position for
     the merge in the left input array, the length of the sequence to
     merge in the left input array, the starting position for the
     merge in the right input array, the length of the sequence to
     merge in the right input array, the starting position for the
     merge in the output array, and the guid for the dependency
     mergelet should satisfy to indicate that it's done. */


  intptr_t *typed_paramv = (intptr_t *) *paramv;
  int left_array_start = (int) typed_paramv[0];
  int left_array_length = (int) typed_paramv[1];
  int right_array_start = (int) typed_paramv[2];
  int right_array_length = (int) typed_paramv[3];
  int output_array_start = (int) typed_paramv[4];
  ocrGuid_t output_event = (ocrGuid_t) typed_paramv[5];

  int *left_input_array = (int *) depv[0].ptr;
  int *right_input_array = (int *) depv[1].ptr;
  int *output_array = (int *) depv[2].ptr;
  /*  std::ostringstream buf;
  buf << "starting mergelet with left_array_start = " << left_array_start << ", left_array_length = " <<  left_array_length << ", right_array_start = " << right_array_start << ", right_array_length = " << right_array_length << "\n";
  std::cerr << buf.str(); */

  /* do the merge */
  int left_index = left_array_start;
  int right_index = right_array_start;
  int output_index = output_array_start;
  

  while((left_index < (left_array_start + left_array_length)) && (right_index < (right_array_start + right_array_length))){
    if(left_input_array[left_index] < right_input_array[right_index]){
      output_array[output_index] = left_input_array[left_index];
      left_index++;
    }
    else{
      output_array[output_index] = right_input_array[right_index];
      right_index++;
    }
    output_index++;
  }
    
  // ok, we've reached the end of at least one of the sub-arrays, so finish up
  while(left_index < (left_array_start + left_array_length)){
    output_array[output_index] = left_input_array[left_index];
    left_index++;
    output_index++;
  }
  while(right_index < (right_array_start + right_array_length)){
    output_array[output_index] = right_input_array[right_index];
    right_index++;
    output_index++;
  }
  //  std::cout << output_event << "\n";
  ocrEventSatisfy(output_event, NULL_GUID);
  delete *paramv; //free our parameters.  Merge_phi will take care of
		//freeing the datablocks we were passed
  delete paramv;
  return(0);
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
