#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_LEN 100

void commonChars(char arr[][MAX_LEN], int n) {
  int common[26];
  for (int i = 0; i < 26; i++) {
    common[i] = true;
  }

  for (int s = 0; s < n; s++) {
		/* create an array of size 26 to store the chars in string  and initialize it to false					 */
    int present[26] = {false}; // 3 points

    for (int i = 0; arr[s][i] != '\0'; i++) { // 3 points- loop through the string correctly
			// set the character true in array (below two lines)
      char c = arr[s][i];                     // 1 point
      present[c - 'a'] = true;                // 2 point
    }

    for (int i = 0; i < 26; i++) {         // 3 points- loop from 0 to 26
      common[i] = common[i] && present[i]; // 5 points - find all the common characters and make them true and rest to false
    }
  }

  printf("Common characters: ");
  int found = false;             // 1 point
  for (int i = 0; i < 26; i++) { // 3 points
    if (common[i]) {             // 2 points
      printf("%c ", i + 'a');    // 3 points
      found = true;              // 1 point
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
    strcpy(arr[i], argv[i + 1]); // simple copy
  }

  commonChars(arr, n);

  return 0;
}
