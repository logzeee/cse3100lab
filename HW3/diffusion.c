#include <stdio.h>
#include <stdlib.h>

void one_particle(int *grid, int n)
{
	int x = 0, y = 0, z = 0;
	int size = 2 * n + 1;
	for(int i = 0; i < n; i++)
	{
		int r = rand() % 6;
		if(r == 0) x--;
		else if(r == 1) x++;
		else if(r == 2) y++;
		else if(r == 3) y--;
		else if(r == 4) z++;
		else z--;
	}
	grid[(z + n) * size * size + (y + n) * size + (x + n)]++;
}

double density(int *grid, int n, double r)
{
	int size = 2 * n + 1;
	double threshold = r * n * r * n;
	int within = 0;
	int total = 0;
	for(int z = -n; z <= n; z++)
	{
		for(int y = -n; y <= n; y++)
		{
			for(int x = -n; x <= n; x++)
			{
				int count = grid[(z + n) * size * size + (y + n) * size + (x + n)];
				total += count;
				if((double)(x * x + y * y + z * z) <= threshold)
					within += count;
			}
		}
	}
	if(total == 0) return 0.0;
	return (double)within / total;
}

void print_result(int *grid, int n)
{
	printf("radius density\n");
	for(int k = 1; k <= 20; k++)
	{
		printf("%.2lf   %lf\n", 0.05*k, density(grid, n, 0.05*k));
	}
}

void diffusion(int n, int m)
{
	int size = 2 * n + 1;
	int *grid = (int *)calloc(size * size * size, sizeof(int));

	for(int i = 1; i <= m; i++) one_particle(grid, n);

	print_result(grid, n);
	free(grid);
}

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		printf("Usage: %s n m\n", argv[0]);
		return 0;
	}
	int n = atoi(argv[1]);
	int m = atoi(argv[2]);

	srand(12345);
	diffusion(n, m);
	return 0;
}
