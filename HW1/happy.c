#include <stdio.h>
#include <stdlib.h>

int main()
{
	int n;

	printf("n = ");
	scanf("%d", &n);

	int m = n;
	//TODO
	//add code below
	while (n != 4 && n != 1) {
		int sum = 0;
		int temp = n;  
		
		
		while (temp > 0) {
			int digit = temp % 10;
			sum += digit * digit;
			temp = temp / 10;
		}
		
		
		printf("%d\n", sum);
		
		
		n = sum;
	}



	if(n==1) printf("%d is a happy number.\n", m);
	else printf("%d is NOT a happy number.\n", m);
	return 0;
}
