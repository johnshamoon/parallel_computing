#include <stdio.h> 
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

// Comparator.
int IncOrder(const void* a, const void* b) {
  return (*((int*)a) - *((int*)b));
}

// Swap method
static inline void swap(int a, int b) {
    int temp = a;
    a = b;
    b = temp;
}

static void quicksort(int *arr, int q, int r) {
  if (q < r) {
    int x = arr[q];
    int s = q;

    int i;
    for (i = q + 1; i < r; i++) {
      if (arr[i] <= x) {
        s++;
        swap(arr[s], arr[i]);
      }
    }
    swap(arr[q], arr[s]);
    quicksort(arr, q, s);
    quicksort(arr, s + 1, r);
  }
}

static void initialize_arr(int* arr, int n) {
  int i;
  arr = malloc(n * sizeof(arr));
  srand(time(0));
  for (i = 0; i < n; i++) {
    arr[i] = rand() % 128 + 1;
  }
}

static void print_to_file_in_ms(FILE* fp, int arr_size,
                                struct timeval start, struct timeval end) {
  unsigned long long t = (end.tv_sec*1e6 + end.tv_usec) -
                         (start.tv_sec*1e6 + start.tv_usec);
  printf("%d time: %llu microseconds\n", arr_size, t);
  fprintf(fp, "%d time: %llu microseconds\n", arr_size, t);
}

int main() { 
  int n = 65536;
  int* arr;
  struct timeval start, end;
  FILE* fp = fopen("quicksort_results.txt", "w");

  gettimeofday(&start, NULL);
  initialize_arr(arr, n);
  quicksort(arr, 0, n - 1);
  gettimeofday(&end, NULL);
  print_to_file_in_ms(fp, n, start, end);
  free(arr);

  n = 1048576;
  gettimeofday(&start, NULL);
  initialize_arr(arr, n);
  quicksort(arr, 0, n - 1);
  gettimeofday(&end, NULL);
  print_to_file_in_ms(fp, n, start, end);
  free(arr);

  n = 16777216;
  gettimeofday(&start, NULL);
  initialize_arr(arr, n);
  quicksort(arr, 0, n - 1);
  gettimeofday(&end, NULL);
  print_to_file_in_ms(fp, n, start, end);
  free(arr);

  n = 1073741824;
  gettimeofday(&start, NULL);
  initialize_arr(arr, n);
  quicksort(arr, 0, n - 1);
  gettimeofday(&end, NULL);
  print_to_file_in_ms(fp, n, start, end);
  free(arr);

  fclose(fp);
  return 0;
} 
