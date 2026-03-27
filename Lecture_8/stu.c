#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int id;
  int points;
} Exam1;

int compareGrades(const void *a, const void *b) {
  Exam1 *pa = (Exam1 *)a;
  Exam1 *pb = (Exam1 *)b;
  return pa->points - pb->points;
}

int main() {
  Exam1 grades[10];

  grades[0] = (Exam1){1, 99};
  grades[1] = (Exam1){2, 95};
  grades[2] = (Exam1){3, 87};
  grades[3] = (Exam1){4, 92};
  grades[4] = (Exam1){5, 78};
  grades[5] = (Exam1){6, 85};
  grades[6] = (Exam1){7, 91};
  grades[7] = (Exam1){8, 73};
  grades[8] = (Exam1){9, 88};
  grades[9] = (Exam1){10, 94};

  printf("Before sorting:\n");
  for (int i = 0; i < 10; i++) {
    printf("ID: %d, Points: %d\n", grades[i].id, grades[i].points);
  }

  // sort the array of grades
  qsort(grades, 10, sizeof(Exam1), compareGrades);

  printf("\nAfter sorting by points (descending):\n");
  for (int i = 0; i < 10; i++) {
    printf("ID: %d, Points: %d\n", grades[i].id, grades[i].points);
  }

  return 0;
}
