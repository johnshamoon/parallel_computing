#include <mpi.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <math.h>

const int array_size = (int)pow(2, 30);

void printResult(int * arrayResult, int sizeofArrRes,
    int origin_or_result, double total_time) {
  FILE *fp;
  int x = 0;
  int fully_sorted = 1;
  int count = 1;
  fp = fopen("odd_even_time.txt", "a");
  fprintf(fp, "Size: %d\n", array_size);
  fprintf(fp, "Computation Execution Time: %f seconds\n", total_time);
  fclose(fp);
}

CompareSplit(int nlocal, int *elmnts, int *relmnts, int *wspace, int keepsmall) {
  int i, j, k;

  for (i=0; i<nlocal; i++)
    wspace[i] = elmnts[i];

  if (keepsmall) {
    for (i=j=k=0; k<nlocal; k++) {
      if (j == nlocal || (i < nlocal && wspace[i] < relmnts[j])) {
        elmnts[k] = wspace[i++];
      } else {
        elmnts[k] = relmnts[j++];
      }
    }
  } else {
    for (i=k=nlocal-1, j=nlocal-1; k>=0; k--) {
      if (j == 0 || (i >= 0 && wspace[i] >= relmnts[j])) {
        elmnts[k] = wspace[i--];
      } else {
        elmnts[k] = relmnts[j--];
      }
    }
  }
}

int IncOrder(const void *e1, const void *e2) {
  return (*((int *)e1) - *((int *)e2));
}


int main (int argc, char *argv[]) {

  int n;
  int npes;
  int rank;
  int nlocal;
  int *elmnts;
  int *relmnts;
  int *allelements;
  int *result_allelements;

  int odd_rank;
  int even_rank;
  int *wspace;
  int i;
  MPI_Status status;

  int x, z;

  struct timespec tstart;
  struct timespec tend;
  uint64_t texecution;
  clock_gettime(CLOCK_MONOTONIC, &tstart);
  printf("Start Computation.\n");

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &npes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  double start_time = MPI_Wtime();

  n = array_size;
  nlocal = n/npes;

  allelements = (int *)malloc(n*sizeof(int));
  result_allelements = (int *)malloc(n*sizeof(int));
  elmnts = (int *)malloc(nlocal*sizeof(int));
  relmnts = (int *)malloc(nlocal*sizeof(int));
  wspace = (int *)malloc(nlocal*sizeof(int));


  if (rank == 0) {
    for (x = 0; x < n; x++) {
      allelements[x] = rand()%128;
      result_allelements[x] = 0;
    }
    for (z = 1; z < npes; z++) {
      for (x = z*nlocal; x < z*nlocal+nlocal; x++) {
        elmnts[x%nlocal] = allelements[x];
      }
      MPI_Send(elmnts, nlocal, MPI_INT, z, 1, MPI_COMM_WORLD);
    }
    for (x = 0; x < nlocal; x++) {
      elmnts[x] = allelements[x];
    }
  } else {
    for (z = 1; z < npes; z++) {
      if (rank == z) {
        MPI_Recv(elmnts, nlocal, MPI_INT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }
  }


  qsort(elmnts, nlocal, sizeof(int), IncOrder);

  if (rank % 2 == 0) {
    odd_rank = rank-1;
    even_rank = rank+1;
  } else {
    odd_rank = rank+1;
    even_rank = rank-1;
  }

  if (odd_rank == -1 || odd_rank == npes) {
    odd_rank = MPI_PROC_NULL;
  }
  if (even_rank == -1 || even_rank == npes) {
    even_rank = MPI_PROC_NULL;
  }

  for (i=0; i<npes-1; i++) {
    if (i%2 == 1) {
      MPI_Sendrecv(elmnts, nlocal, MPI_INT, odd_rank, 1, relmnts, nlocal, MPI_INT, odd_rank, 1, MPI_COMM_WORLD, &status);
    }
    else {
      MPI_Sendrecv(elmnts, nlocal, MPI_INT, even_rank, 1, relmnts, nlocal, MPI_INT, even_rank, 1, MPI_COMM_WORLD, &status);
    }

    CompareSplit(nlocal, elmnts, relmnts, wspace, rank < status.MPI_SOURCE);
  }


  if (rank == 0) {
    for (x = 0; x < nlocal; x++) {
      result_allelements[x] = elmnts[x]
    }
    for (z = 1; z < npes; z++) {
      MPI_Recv(relmnts, nlocal, MPI_INT, z, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      for (x = z*nlocal; x < z*nlocal+nlocal; x++) {
        result_allelements[x] = relmnts[x%nlocal];
      }
    }
  } else {
    for (z = 1; z < npes; z++) {
      if (rank == z) {
        MPI_Send(elmnts, nlocal, MPI_INT, 0, 2, MPI_COMM_WORLD);
      }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    double elapsed_time = MPI_Wtime() - start_time;
    printResult(result_allelements, n, 1, elapsed_time);
  }

  free(result_allelements);
  free(allelements);
  free(elmnts);
  free(relmnts);
  free(wspace);
  MPI_Finalize();
}


