// 2d array or size 3 rows and 5 cols using malloc

#include <stdio.h>
#include <stdlib.h>

int main() {

  int **rows = malloc(3 * sizeof(int *));
  /* rows[0] = malloc(5*sizeof(int)); */
  /* rows[1] = malloc(5*sizeof(int)); */
  /* rows[2] = malloc(5*sizeof(int)); */

  for (int i = 0; i < 3; i++) {

    rows[i] = malloc(5 * sizeof(int));
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      printf("Enter [%d][%d]:", i, j);
      scanf("%d", &rows[i][j]);
    }
  }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 5; j++) {
      printf("[%d][%d]:%d ", i, j, rows[i][j]);
    }
    printf("\n");
  }

  for (int i = 0; i < 3; i++) {
    free(rows[i]);
  }
  free(rows);

  return 0;
}
