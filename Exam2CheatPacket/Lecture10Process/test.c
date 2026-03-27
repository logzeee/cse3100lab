#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
	printf("hola\n");
  int a = fork();
  if (a == 0) {

		printf("a is child %d\n",a);
    printf("This is in child,PID:%d\n",getpid());
		printf("Hello from child\n");
  }
	if(a>0) {
		int status;
		printf("a is parent %d\n",a);
		wait(&status);
    printf("This is in parent,PID:%d\n",getpid());
  }

	printf("Hello\n");
}
