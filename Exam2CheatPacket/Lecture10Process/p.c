#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  int a = fork();
  int b = fork();

  printf("a = %d,b=%d,pid = %d\n", a, b, getpid());
  if (a > 0 && b > 0) {
		waitpid(a,NULL,0);
		waitpid(b,NULL,0);
    printf("The main process 1, the god father\n");
  }
	if(a==0){
		if(b==0){
			printf("This is process 4\n");
		}
		if(b>0){
			printf("This is process 2\n");
		}
	}
	if(a>0 && b==0){
		printf("This is process 3\n");
	}	
  return 0;
}
