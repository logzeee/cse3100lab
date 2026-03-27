#include <stdio.h>
#include <unistd.h>

int main() {
  int a = fork();
  int b = fork();
  if (a == 0) {

		printf("a is child %d\n",a);
		printf("b is child %d\n",b);
    printf("This is in child,PID:%d\n",getpid());
  }
	if(a>0) {
		printf("a is parent %d\n",a);
		printf("b is parent %d\n",b);
    printf("This is in parent,PID:%d\n",getpid());
  }
}
