#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "sequential_mergesort.h"

#define ARRAY_SIZE 16384

main(){
  //  srand(time(NULL)); /* Init RNG */
  int *sort_array, *temp_array, i;
  
  sort_array = new int[ARRAY_SIZE];
  temp_array = new int[ARRAY_SIZE];
  for (i = 0; i < ARRAY_SIZE; i++){
    sort_array[i] = rand();

  }
  
  
  /* std::cout << "Starting  Array\n";
  for (i = 0; i < ARRAY_SIZE; i ++){
    std::cout << sort_array[i] << " ";
    }
    std::cout << "\n"; */ 

  /* Call the actual sort routine */
  sequential_mergesort(sort_array, temp_array, 0, ARRAY_SIZE-1);
  
  /*  std::cout << "Sorted Array\n";
  for (i = 0; i < ARRAY_SIZE; i ++){
    std::cout << sort_array[i] << " ";
  }
  std::cout << "\n"; */

  /* Check the result */
  int bad_sort = 0;
  for(i = 1; i < ARRAY_SIZE; i++){
    if(sort_array[i-1] > sort_array[i]){
      std::cout << "Mis-sorted value found at positions " << i-1 << " and " << i << "\n";
      bad_sort = 1;
    }
  }
  if (bad_sort == 0){
    std::cout << "Sorted array checks out\n";
    }
}
