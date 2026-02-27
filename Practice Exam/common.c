// Do not modify starter code
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_LEN 100

void commonChars(char arr[][MAX_LEN], int n) {
  int common[26];
  for (int i = 0; i < 26; i++) {
    common[i] = true;
  }

  for (int i = 0; i < n; i++) {

    // fill code here
  }

  printf("Common characters: ");

  // fill code here
  if (!found) {
    printf("None");
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s string1 string2 ...\n", argv[0]);
    return 1;
  }

  int n = argc - 1;
  char arr[n][MAX_LEN];

  for (int i = 0; i < n; i++) {
    strcpy(arr[i], argv[i + 1]);
  }

  commonChars(arr, n);

  return 0;
}
