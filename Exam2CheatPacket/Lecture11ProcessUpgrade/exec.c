#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  printf("This file will run some other c file\n");
 
	execl("./calc", "./calc", "product", "1", "2", "3", "4", NULL);
  
	/* printf("Done with that other file\n"); */

  return 0;
}
