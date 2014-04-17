/*
 * sequential-mergesort.cpp
 *
 *  Created on: Jul 17, 2011
 *      Author: npcarter
 */

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "sequential_mergesort.h"

void sequential_mergesort(int *sort_array, int start_index, int end_index){
  int *temp_array, i, j, k, new_chunk_size, j_start;

  temp_array = new int[ARRAY_SIZE];
  /* std::cout << "Sequential_mergesort(" << sort_array << " " <<
     start_index << " " << end_index << ")\n"; */

  if (end_index == start_index) {
    /* We've hit the bottom of the recursion tree, and one-element
       lists are always sorted */
    return;
  }

  /* Otherwise, recurse*/
  new_chunk_size = ((end_index+1) -start_index)/2;  /* +1 because of
						       zero-indexed arrays */
  sequential_mergesort(sort_array, start_index, (start_index+new_chunk_size)-1);
  sequential_mergesort(sort_array, start_index + new_chunk_size, end_index);
  
  /* After we get back from recursion, do the merge */
  
  
  /* Find the midpoint of the current chunk */
  j = start_index + (((end_index +1) - start_index)/2);
  j_start = j;

  for (i = start_index; i < j_start; i++){
    temp_array[i] = sort_array[i];
  }

  k = start_index;
  i = start_index;
  
  while((i< j_start) && (j <= end_index)){
    if (temp_array[i] < sort_array[j]){
      sort_array[k] = temp_array[i];
      i++;
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
  if (i < j_start){
    while(k <= end_index){
      sort_array[k] = temp_array[i];
      i++;
      k++;
    }
  }
  /*
  for (i = 0; i < ARRAY_SIZE; i ++){
    std::cout << sort_array[i] << " ";
  }
  std::cout << "\n";
  */
  return;
}



