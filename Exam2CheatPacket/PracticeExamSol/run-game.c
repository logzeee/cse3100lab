#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// write an integer to a pipe
void write_int(int pd, int value) { write(pd, &value, sizeof(int)); }
// read an integer from a pipe
// the function returns the number of bytes read
int read_int(int pd, int *value) { return read(pd, value, sizeof(int)); }

double random_double() { return (double)rand() / RAND_MAX; }

int coin_flip(double p) { return random_double() < p; }

void run_simulation(int n, double p) {
  int pd1[2];
  // pipe creation
  if (pipe(pd1) < 0) {
    perror("Error.");
    exit(-1);
  }

  int pd2[2];
  // pipe creation
  if (pipe(pd2) < 0) {
    perror("Error.");
    exit(-1);
  }
  pid_t pid = fork();
  if (pid == 0) {
    // TODO
    // fill in code below
    // note this is player A

    close(pd1[0]); // 1 point
    close(pd2[1]); // 1 point

    srand(3100);

    int v;
    // TODO
    // complete the following line of code
    while (read_int(pd2[0], &v) != 0) { // 1 point for filling
      // TODO
      // fill in code below
      write_int(pd1[1], coin_flip(p)); // 5 points
    }
    // TODO
    // fill in code below

    close(pd1[1]); // 1 point
    close(pd2[0]); // 1 point

    exit(EXIT_SUCCESS); // 1 point
  }
  // TODO
  // fill in code below

  close(pd1[1]); // 1 point
  close(pd2[0]); // 1 point

  int pd3[2];
  // pipe creation
  if (pipe(pd3) < 0) {
    perror("Error.");
    exit(-1);
  }

  int pd4[2];
  // pipe creation
  if (pipe(pd4) < 0) {
    perror("Error.");
    exit(-1);
  }
  pid_t pid1 = fork();

  if (pid1 == 0) {
    // TODO
    // fill in code below
    // note this is player B

    close(pd3[0]); // 1 point
    close(pd4[1]); // 1 point

    close(pd1[0]); // 1 point
    close(pd2[1]); // 1 point

    srand(3504);

    int v;
    // TODO
    // complete the following line of code
    while (read_int(pd4[0], &v) != 0) { // 1 point
      // TODO
      // fill in code below
      write_int(pd3[1], coin_flip(p)); // 5 points
    }

    // TODO
    // fill in code below

    close(pd3[1]); // 1 point
    close(pd4[0]); // 1 point

    exit(EXIT_SUCCESS); // 1 point
  }

  close(pd3[1]); // 1 point
  close(pd4[0]); // 1 point

  int n1 = 0;
  int n2 = 0;
  int v1, v2;

  while (n1 + n2 < n) {
    // TODO
    // finish the follow lines of code
    write_int(pd2[1], 1);  // 1 point
    write_int(pd4[1], 1);  // 1 point
    read_int(pd1[0], &v1); // 1 point
    read_int(pd3[0], &v2); // 1 point
    if (v1)
      n1++;
    else {
      if (v2)
        n2++;
    }
  }
  printf("prob of A wins = %.3lf\n", (double)(n1) / (n1 + n2));
  // TODO
  // fill in code below

  close(pd1[0]); // 1 point
  close(pd2[1]); // 1 point
  close(pd3[0]); // 1 point
  close(pd4[1]); // 1 point
}
