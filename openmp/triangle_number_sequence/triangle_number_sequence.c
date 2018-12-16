#include <stdio.h>
#include <omp.h>
#define SIZE_OF_ARR 16384
#define DEBUG 0

int arr[SIZE_OF_ARR];

void initialize() {
  int i;
  #pragma omp parallel for
  for (i = 0; i < SIZE_OF_ARR; i++) {
    arr[i] = i;
  }
}

void phase_1() {
  int i;
  #pragma omp parallel for
  for (i = 1; i < SIZE_OF_ARR; i++) {
    arr[i] = arr[i] + i - 1;
  }
}

void phase_2() {
  int i;
  int temp[SIZE_OF_ARR];
  temp[0] = arr[0];
  temp[1] = arr[1];
  #pragma omp parallel for
  for (i = 2; i < SIZE_OF_ARR; i++) {
    temp[i] = arr[i - 2] + arr[i];
  }
  #pragma omp parallel for
  for (i = 0; i < SIZE_OF_ARR; i++) {
    arr[i] = temp[i];
  }
}

void phase_3() {
  int i;
  int temp[SIZE_OF_ARR];
  temp[0] = arr[0];
  temp[1] = arr[1];
  temp[2] = arr[2];
  temp[3] = arr[3];
  #pragma omp parallel for
  for (i = 4; i < SIZE_OF_ARR; i++) {
    temp[i] = arr[i - 4] + arr[i];
  }
  #pragma omp parallel for
  for (i = 0; i < SIZE_OF_ARR; i++) {
    arr[i] = temp[i];
  }
}

int main(int argc, char** argv) {
  double start_time = omp_get_wtime();
  initialize();
  phase_1();
  phase_2();
  phase_3();
  double end_time = omp_get_wtime();
  double time = end_time - start_time;
  printf("Size of Array: %d\n", SIZE_OF_ARR);
  printf("Time in Microseconds: %f\n", time * 1000000);

#if DEBUG
  int i;
  for (i = 0; i < SIZE_OF_ARR; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");
#endif
  return 0;
}
