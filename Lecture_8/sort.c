#include <stdio.h>
#include <stdlib.h>

// comparison function
// should take two elements a and b
// if a<b return -1
// if a==b return 0
// if a>b return 1	
//

int integerCompare(const void *a, const void *b) {
  /* int *ia = (int *)a; */
  /* int *ib = (int *)b; */

  /* if (*ia < *ib) */
  /*   return -1; */
  /* else if (*ia == *ib) */
  /*   return 0; */
  /* else */
  /*   return 1; */
	// we can either do all the steps above or
	// the below steps will also achieve the same
	return *(int*)a - *(int*)b;
}

int main() {

  int a[10] = {4, 2, 67, 12, -4, 43, 100, 23, 98, 0};

  printf("Elements before sorting:\n");
  for (int i = 0; i < 10; i++) {
    printf("%d\n", a[i]);
  }

  // n = 10;
  // size = sizeof(int)
  // base = a
	// comparator function = integerCompare
	
	// qsort(base,n,size,comparator function)
	
  qsort(a, 10, sizeof(int), integerCompare);

  printf("Elements before sorting:\n");
  for (int i = 0; i < 10; i++) {
    printf("%d\n", a[i]);
  }
  return 0;
}
