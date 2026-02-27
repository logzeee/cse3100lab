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
    int seen[26];
    for (int j = 0; j < 26; j++)
      seen[j] = false;

    for (int j = 0; j < strlen(arr[i]); j++)
      seen[arr[i][j] - 'a'] = true;

    for (int j = 0; j < 26; j++)
      if (!seen[j])
        common[j] = false;
  }

  printf("Common characters: ");

  bool found = false;
  for (int i = 0; i < 26; i++) {
    if (common[i]) {
      printf("%c ", 'a' + i);
      found = true;
    }
  }
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
