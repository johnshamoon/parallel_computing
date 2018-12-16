// John Shamoon - fw3860
// Homework 5
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define MATRIX_SIZE 128
#define NUM_NODES 8
#define MASTER 0

int matrix_a[MATRIX_SIZE][MATRIX_SIZE];
int matrix_b[MATRIX_SIZE][MATRIX_SIZE];

void initialize_matrix() {
  int i, j;
  for (i = 0; i < MATRIX_SIZE; i++) {
    for (j = 0; j < MATRIX_SIZE; j++) {
      matrix_a[i][j] = j + 1;
      matrix_b[i][j] = j + 1;
    }
  }
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

void print_matrix_to_file() {
  FILE* fp;
  fp = fopen("john_shamoon_matrix_output.txt", "w");
  int i, j;
  for (i = 0; i < MATRIX_SIZE; i++) {
    for (j = 0; j < MATRIX_SIZE; j++) {
      fprintf(fp, "%d ", matrix_a[i][j]);
    }
    fprintf(fp, "\n");
  }

  fclose(fp);
}

int main(int argc, char** argv) {
  int i, j;
  int node;
  // Number used as a parameter filler to signal other nodes.
  int number = -1;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &node);

  if (node == MASTER) {
    initialize_matrix();

    // Send each node its rows.
    for (i = 1; i < NUM_NODES; i++) {
      for (j = i*16; j <= i*16 + 15; j++) {
        MPI_Send(&matrix_a[j][0], MATRIX_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Send(&matrix_b[j][0], MATRIX_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
      }
    }

    // Compute the master's summation.
    for (i = 0; i <= 15; i++) {
      for (j = 0; j < MATRIX_SIZE; j++) {
        matrix_a[i][j] += matrix_b[i][j];
      }
    }
    printf("Process %d: Done\n", node);

    // Block and collect the summations from the other nodes.
    for (i = 1; i < NUM_NODES; i++) {
      for (j = i*16; j <= i*16 + 15; j++) {
        MPI_Recv(&matrix_a[j][0], MATRIX_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }

    // Print to a file using fprintf.
    print_matrix_to_file();

    // Signal node 1 that it can print.
    MPI_Send(&number, 1, MPI_INT, 1, 1, MPI_COMM_WORLD);
  } else {
    // Receive the rows to be computed.
    for (i = node*16; i <= node*16 + 15; i++) {
      MPI_Recv(&matrix_a[i][0], MATRIX_SIZE, MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&matrix_b[i][0], MATRIX_SIZE, MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Compute the summations for each of the rows in this set.
    for (i = node*16; i <= node*16 + 15; i++) {
      for (j = 0; j < MATRIX_SIZE; j++) {
        matrix_a[i][j] += matrix_b[i][j];
      }
    }

    // Send the local result back to the master node.
    for (i = node*16; i <= node*16 + 15; i++) {
      MPI_Send(&matrix_a[i][0], MATRIX_SIZE, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
    }

    // Block and wait for the previous node to signal that we can print.
    MPI_Recv(&number, 1, MPI_INT, node - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Process %d: Done\n", node);
    if (node < NUM_NODES - 1) {
      MPI_Send(&number, 1, MPI_INT, node + 1, 1, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
}
