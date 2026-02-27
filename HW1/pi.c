#include <stdio.h>
#include <stdlib.h>

int main()
{
	int n, i;

	printf("n = ");
	scanf("%d", &n);

	double pi = 0.;
	double power = 1.0;  
	//TODO
	//add code below 
	for (i=0; i<n+1; i++) {
		pi += (
			4.0 / (8*i +1)
			- 2.0 / (8*i +4)
			- 1.0 / (8*i +5)
			- 1.0 / (8*i +6)
		) * power;
		power /= 16.0;  
	}


	printf("PI = %.10f\n", pi);
	return 0;
}
