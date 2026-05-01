// we can search using a pattern.
// for example, we can search w-rd
// and we will find ward and word
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_WORD_COUNT 60000 // we have less than 60000 words
#define MAX_WORD_LENGTH 80   // each word is less than 80 letters

char words[MAX_WORD_COUNT][MAX_WORD_LENGTH]; // 2-d array to hold all the words
int count = 0;                               // number of words, initilized to 0

struct thread_data {
  int thread_num;
  char pattern[MAX_WORD_LENGTH]; // pattern for the search
  int i, j;                      // the staring and ending index
};

// read words from the file to the array words declared above
// also update the number of words (update variable count)
void read_file_to_array(char *filename) {
  FILE *fp;

  // open the file for reading
  fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Cannot open the file.\n");
    exit(-1);
  }
  // fill in the code below
  // make sure when each word is saved in the array words,
  // it does not ends with a '\n'
  while (!feof(fp)) {
    fscanf(fp, "%s\n", words[count]);
    count++;
  }
  fclose(fp);
}

int check_word(const char *pattern, const char *word) {
  if (strlen(pattern) != strlen(word))
    return 0;
  for (int i = 0; i < strlen(word); i++) {
    if (pattern[i] != '-' && word[i] != pattern[i])
      return 0;
  }
  return 1;
}

void *thread_search(void *threadarg) {
  struct thread_data *my_data = (struct thread_data *)threadarg;
  // printf("%d %d\n", my_data->i, my_data->j);
  for (int k = my_data->i; k <= my_data->j; k++) {
    if (check_word(my_data->pattern, words[k]))
      printf("%s\n", words[k]);
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s pattern NUM_THREADS\n", argv[0]);
    exit(-1);
  }
  int NUM_THREADS = atoi(argv[2]);
  assert(NUM_THREADS >= 1 && NUM_THREADS <= 10);

  read_file_to_array("dict.txt");
  pthread_t threads[NUM_THREADS];
  struct thread_data thread_data_array[NUM_THREADS];
  int rc, t;
  for (t = 0; t < NUM_THREADS; t++) {
    thread_data_array[t].thread_num = t;
    strcpy(thread_data_array[t].pattern, argv[1]);
    thread_data_array[t].i = t * count / NUM_THREADS;
    thread_data_array[t].j = (t + 1) * count / NUM_THREADS - 1;
    rc =
        pthread_create(&threads[t], NULL, thread_search, &thread_data_array[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }
  for (t = 0; t < NUM_THREADS; t++) {
    rc = pthread_join(threads[t], NULL);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }
  return 0;
}
