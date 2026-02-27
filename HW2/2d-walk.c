#include <stdio.h>
#include <stdlib.h>

double two_d_random(int n)
{
	int size = 2 * n - 1;
	int total = size * size;
	int *visited = (int *)calloc(total, sizeof(int));

	int x = 0, y = 0;
	visited[(y + n - 1) * size + (x + n - 1)] = 1;

	while(1) {
		int r = rand() % 4;
		if(r == 0) y--;       
		else if(r == 1) x++;  
		else if(r == 2) y++;  
		else x--;            

		if(x == n || x == -n || y == n || y == -n) break;

		visited[(y + n - 1) * size + (x + n - 1)] = 1;
	}

	int count = 0;
	int i;
	for(i = 0; i < total; i++)
		if(visited[i]) count++;

	free(visited);
	return (double)count / total;
}

//Do not change the code below
int main(int argc, char *argv[])
{
	int trials = 1000;
	int i, n, seed;
	if (argc == 2) seed = atoi(argv[1]);
	else seed = 12345;

	srand(seed);
	for(n=1; n<=64; n*=2)
	{	
		double sum = 0.;
		for(i=0; i < trials; i++)
		{
			double p = two_d_random(n);
			sum += p;
		}
		printf("%d %.3lf\n", n, sum/trials);
	}
	return 0;
}
