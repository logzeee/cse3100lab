#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int count_all_odd(char *s) {
  int count = 0;
  int len = strlen(s);
  for (int i = 0; i < len; i++) {
    int digit = s[i] - '0';
    if (digit % 2 == 1) {
      count++;
    }
  }
  return count;
}

int compare_odd_count(const void *a, const void *b) {
  char *s1 = *(char **)a;
  char *s2 = *(char **)b;
  return count_all_odd(s1) - count_all_odd(s2);
}

void print_elements(char **elems, int count) {
  for (int i = 0; i < count; i++) {
    printf("%s\n", elems[i]);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    perror("Usage : ./place <strings>");
    exit(1);
  }

  qsort(&argv[1], argc - 1, sizeof(char *), compare_odd_count);

  print_elements(&argv[1], argc - 1);

  return 0;
}
