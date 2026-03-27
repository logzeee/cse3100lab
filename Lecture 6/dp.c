#include <stdio.h>

int main() {
  int a = 353;

	int *ip = &a;
	char *cp = &a;

	printf("ip  = %p\n",ip);
	printf("cp  = %p\n",cp);

	printf("*ip  = %d\n",*ip);
	printf("*cp  = %c\n",*cp);
	printf("*cp  = %d\n",*cp);

  return 0;
}
