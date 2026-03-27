#include <stdio.h>


void fun1(int *p){
	*p = *p+1;
}

int *fun2(int *p){
	/* printf("value inside p  = %p\n",p); */
	*p = *p+1;
	return p;
}

int *fun3(int *p){
	int res = *p+1;
	return &res;
}

int main() {
  int a = 10;

	printf("a in main before fun1():%d\n",a);
	fun1(&a);
	printf("a in main after fun1():%d\n",a);
	
	/* printf("address of a = %p\n",&a); */
	int *res = fun2(&a);
	/* printf("value inside res  = %p\n",res); */
	printf("*res = %d\n",*res);

	int *r = fun3(&a);
	printf("*r = %d\n",*r);
  return 0;
}
