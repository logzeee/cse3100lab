#include<stdio.h>

#include<stdlib.h>
// 10,20,30,40

void print_array(int *arr,int n){
	for(int i=0;i<n;i++){
		printf("%d\n",arr[i]);
	}
}


int main(){
	/* int *arr = malloc(16); */
	int *arr = malloc(4*sizeof(int));
	*arr = 10;
	arr[0] = 10;
	arr[1] = 20;
	arr[2] = 30;
	arr[3] = 40;
	
	printf("%d\n",*arr);
	printf("%d\n",*(arr+1));

	
}
