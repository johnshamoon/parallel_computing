#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#define MATRIX_SIZE 128
#define NUM_THREADS 8

int matrix_a[MATRIX_SIZE][MATRIX_SIZE];
int matrix_b[MATRIX_SIZE][MATRIX_SIZE];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond[NUM_THREADS] = PTHREAD_COND_INITIALIZER;

int count = 0;

void initialize_matrices() {
  int i, j;
  for (i = 0; i < MATRIX_SIZE; i++) {
    for (j = 0; j < MATRIX_SIZE; j++) {
      matrix_a[i][j] = j + 1;
      matrix_b[i][j] = j + 1;
    }
  }
}

void* add_matrices(void* arg) {
  int turn = *(int*)arg;
  int start = turn * 16;
  int i, j;
  for (i = start; i <= start + 15; i++) {
    for (j = 0; j < MATRIX_SIZE; j++) {
      matrix_a[i][j] = matrix_a[i][j] + matrix_b[i][j];
    }
  }

  // Lock due to changing count.
  pthread_mutex_lock(&mutex);
  // Wait for the threads turn.
  if (turn != count) {
    pthread_cond_wait(&cond[turn], &mutex);
  }

  printf("Thread %d: Done.\n", turn);

  if (count < NUM_THREADS - 1) {
    count++;
  }

  // Signal to the next thread that it is its turn.
  pthread_cond_signal(&cond[count]);
  pthread_mutex_unlock(&mutex);

  return NULL;
}

void print_matrix() {
  int i, j;
  for (i = 0; i < MATRIX_SIZE; i++) {
    for (j = 0; j < MATRIX_SIZE; j++) {
      printf("%d ", matrix_a[i][j]);
    }
    printf("\n");
  }
}

int main(int argc, char** argv) {
  struct timeval start, end;
  int i;
  int tid[NUM_THREADS];
  pthread_t threads[NUM_THREADS];

  // Start timer.
  gettimeofday(&start, NULL);

  initialize_matrices();

  // Create threads to sum rows 16*tid[i] to 16*tid[i] + 1.
  for(i = 0; i < NUM_THREADS; i++) {
    tid[i] = i;
    pthread_create(&threads[i], NULL, add_matrices, &tid[i]);
  }

  // Join all the threads with the main thread.
  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  // End timer.
  gettimeofday(&end, NULL);

  print_matrix();
  printf("Microseconds: %ld\n", end.tv_usec - start.tv_usec);

  return 0;
}
