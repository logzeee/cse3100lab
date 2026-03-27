#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void add(int *a, int size) {
  int result = 0;
  for (int i = 0; i < size; i++) {
    result += a[i];
  }
  printf("Sum is %d\n", result);
}

void product(int *a, int size) {
  int result = 1;
  for (int i = 0; i < size-1; i++) {
    result *= a[i];
  }
  printf("Product is %d\n", result);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "usage:./calc <sum/product> <values>\n");
    return 1;
  }
  int a[argc - 2];
  for (int i = 0; i < argc - 2; i++) {
    a[i] = atoi(argv[i + 2]);
  }
  if (strcmp(argv[1], "sum") == 0) {
    add(a, argc - 2);
  } else {
    product(a, argc - 2);
  }
  return 0;
}
