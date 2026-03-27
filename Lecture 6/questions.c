#include <stdio.h>
#include <stdlib.h>

int main() {
  int a[] = {1, 2, 3, 4};
  int b[4] = {1, 2, 3};

  int i;
  for (i = 0; i < 4; i++) {
    printf("i = %d a[i] = %d b[i] = %d\n", i, a[i], b[i]);
  }

  char c[] = {'a', 'b', 'c', 'd'};
  char d[] = "abcd";

  printf(" c size = %lu d size = %lu\n", sizeof(c), sizeof(d));

  int e[10];

  printf("%p %p\n", &(e[0]), &(e[4]));
  printf("%ld \n", &(e[0])-&(e[4]));

  int t[10][20];
  printf("%p %p\n", &(t[0][0]), &(t[1][1]));
  printf("%ld\n", &(t[0][0]) - &(t[1][1]));
  printf("%ld\n", sizeof(t[0][0]));
  return 0;
}
