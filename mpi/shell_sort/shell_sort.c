// John Shamoon
// CSC 6220 - Term Project
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MASTER 0

void print_array_to_file(int* arr, int arr_size, double total_time);
void shell_sort(int* arr, int n);

int get_hypercube_swap_node(int rank1, int iteration, int num_processors);
void hypercube_compare_exchange(int node, int num_processors,
                                int* temp_buffer_1, int* temp_buffer_2,
                                int partition_size);
void compare_split(int* arr1, int* arr2, int rank1, int rank2, int sizearray);

int even_iteration(int node, int num_processors,
                   int* temp_buffer_1, int* temp_buffer_2, int partition_size);
int odd_iteration(int node, int num_processors,
                  int* temp_buffer_1, int* temp_buffer_2, int partition_size);

int odd_even_iteration(int* arr1, int* arr2, int rank1, int rank2,
                       int sizearray);
void odd_even_transposition(int node, int num_processors,
                            int* arr, int* temp_buffer_1,
                            int* temp_buffer_2, int partition_size);

/**
 * The driver program. Generates an array, fills it with random variables, and
 * uses MPI to shellsort it in parallel.
 *
 * \param argc The command line argument count.
 * \param argv The command line argument vector.
 */
int main (int argc, char** argv) {
  int num_processors, node;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_processors);
  MPI_Comm_rank(MPI_COMM_WORLD, &node);
  double start_time = MPI_Wtime();

  int arr_size = pow(2, 7);
  int partition_size = arr_size / num_processors;

  int* arr = (int*)malloc(arr_size*sizeof(int));
  int* temp_buffer_1 = (int*)malloc(partition_size*sizeof(int));
  int* temp_buffer_2 = (int*)malloc(partition_size*sizeof(int));
  int i;

  // Initialization.
  if (node == MASTER) {
    srand(time(0));
    for (i = 0; i < arr_size; i++) {
      arr[i] = rand() % 129;
    }
    // Distribute the array amongst the other processors, keeping the first
    // chunk for the master.
    for (i = 1; i < num_processors; i++) {
      MPI_Send(arr + (i * partition_size), partition_size,
               MPI_INT, i, 0, MPI_COMM_WORLD);
    }
    for (i = 0; i < partition_size; i++) {
      temp_buffer_1[i] = arr[i];
    }
    shell_sort(temp_buffer_1, partition_size);
  } else {
    MPI_Recv(temp_buffer_1, partition_size,
             MPI_INT, MASTER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    shell_sort(temp_buffer_1, partition_size);
  }

  // Phase I.
  hypercube_compare_exchange(node, num_processors, temp_buffer_1, temp_buffer_2,
                             partition_size);

  // Phase II.
  odd_even_transposition(node, num_processors, arr, temp_buffer_1,
                         temp_buffer_2, partition_size);

  MPI_Barrier(MPI_COMM_WORLD);
  if (node == MASTER) {
    double elapsed_time = MPI_Wtime() - start_time;
    print_array_to_file(arr, arr_size, elapsed_time);
  }

  free(arr);
  free(temp_buffer_1);
  free(temp_buffer_2);

  MPI_Finalize();

  return 0;
}

/**
 * Prints an array to a file called sorted_array.txt.
 *
 * \param arr The array to print.
 * \param arr_size The size of the array.
 * \param total_time The total MPI execution time.
 */
void print_array_to_file(int* arr, int arr_size, double total_time) {
  FILE *fp = fopen("sorted_array.txt", "w");
  int unsorted = 0;
  int i;

  fprintf(fp, "Size: %d\n", arr_size);
  fprintf(fp, "Total Execution Time: %f seconds\n", total_time);
  for (i = 1; i < arr_size; i++) {
    if (arr[i - 1] > arr[i]) {
      unsorted = 1;
    }
    fprintf(fp, "%d\n", arr[i-1]);
  }

  if (unsorted) {
    fprintf(fp, "UNSORTED\n");
  } else {
    fprintf(fp, "SORTED\n");
  }
  fclose(fp);
}

/**
 * Shell sorts an array in ascending order.
 *
 * \arr arr The array to sort.
 * \arr n The size of the array.
 */
void shell_sort(int* arr, int n) {
  int i, j;
  for (i = n / 2; i > 0; i /= 2) {
    for (j = i; j < n; j++) {
      int temp = arr[j];
      int r;
      for (r = j; r >= i && arr[r - i] > temp; r -= i) {
        arr[r] = arr[r - i];
      }
      arr[r] = temp;
    }
  }
}

/**
 * Combines the two provided arrays, sorts them, and distributes half of the
 * elements to each array. The array belonging to the processor with the lower
 * rank receives the array with the smaller elements. The array belonging to the
 * processor with the higher rank recieves the array with the larger elements.
 *
 * \param arr1 The array from the first processor.
 * \param arr2 The array from the second processor.
 * \param rank1 The rank of the first processor.
 * \param rank2 The rank of the second processor.
 * \param arr_size The size of each of the arrays.
 */
void compare_split(int* arr1, int* arr2, int rank1, int rank2, int arr_size) {
  int i;
  int* temp_arr = (int *)malloc(2 * arr_size * sizeof(int));
  // Merge both buffers into a single array.
  for (i = 0; i < arr_size * 2; i++) {
    if (i < arr_size) {
      temp_arr[i] = arr1[i];
    } else {
      temp_arr[i] = arr2[i - arr_size];
    }
  }

  shell_sort(temp_arr, arr_size * 2);

  // Lower rank takes the smaller half, higher rank takes the larger half.
  if (rank1 < rank2) {
    for (i = 0; i < arr_size; i++) {
      arr1[i] = temp_arr[i];
    }
  } else {
    for (i = 0; i < arr_size; i++) {
      arr1[i] = temp_arr[i + arr_size];
    }
  }
}


/**
 * Gets the rank of the processor to swap arrays with. The function takes into
 * consideration the layout of the processors (hypercube) and chooses the 
 * processor accordingly.
 * 
 * \param rank1 The rank of the processor to find a swap processor for.
 * \param iteration The current iteration of this method.
 * \param num_processors The number of processors used for execution.
 */
int get_hypercube_swap_node(int rank1, int iteration, int num_processors) {
  // Bits that need to flip.
  int i, k;
  int rank2 = 0;
  for (i = 0; i <= iteration; i++) {
    if ((rank1 & (int)pow(2, i)) != pow(2, i)) {
      rank2 += pow(2, i);
    }
  }
  for (k = log2(num_processors) - 1; k > iteration; k--) {
    rank2 += (rank1 & (int)pow(2, k));
  }

  return rank2;
}

/**
 * Handles the even iterations of the odd-even transposition. If the node is
 * even, the processor swaps with the processor whose rank is one less than the
 * node. If the node is odd, the processor swaps with the processor whose rank
 * is one higher than the node.
 *
 * \param node The node's rank.
 * \param num_processors The number of processors.
 * \param temp_buffer_1 The buffer used to send.
 * \param temp_buffer_2 The buffer used to receive.
 * \param partition_size The size of the temporary buffers.
 */
int even_iteration(int node, int num_processors,
                   int* temp_buffer_1, int* temp_buffer_2,
                   int partition_size) {
  int is_sorting_complete = 0;
  int rank1, rank2;
  if (!(node % 2)) {
    rank1 = node;
    rank2 = node + 1;
    // Swap data with the paired processor.
    MPI_Sendrecv(temp_buffer_1, partition_size, MPI_INT, rank2, 4,
                 temp_buffer_2, partition_size, MPI_INT, rank2, 4,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    is_sorting_complete = odd_even_iteration(temp_buffer_1, temp_buffer_2,
                                             rank1, rank2, partition_size);
  } else {
    rank1 = node;
    rank2 = node - 1;
    // Swap data with the paired processor.
    MPI_Sendrecv(temp_buffer_1, partition_size, MPI_INT, rank2, 4,
                 temp_buffer_2, partition_size, MPI_INT, rank2, 4,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    is_sorting_complete = odd_even_iteration(temp_buffer_1, temp_buffer_2,
                                             rank1, rank2, partition_size);
  }

  return is_sorting_complete;
}

/**
 * Handles the odd iterations of the odd-even transposition. If the node's rank
 * is even and is not the highest ranked node, it swaps with the node whose rank
 * is one higher than its own. If the node's rank is odd and the node is not the
 * root, it swaps with the node whose rank is one less than its own.
 *
 * \param node The node's rank.
 * \param num_processors The number of processors.
 * \param temp_buffer_1 The buffer used to send.
 * \param temp_buffer_2 The buffer used to receive.
 * \param partition_size The size of the temporary buffers.
 */
int odd_iteration(int node, int num_processors,
                  int* temp_buffer_1, int* temp_buffer_2,
                  int partition_size) {
  int is_sorting_complete = 0;
  int rank1, rank2;
  // If odd and not the last processor.
  if ((node % 2) && node != num_processors - 1) {
    rank1 = node;
    rank2 = node + 1;
    // Swap data with the paired processor.
    MPI_Sendrecv(temp_buffer_1, partition_size, MPI_INT, rank2, 4,
                 temp_buffer_2, partition_size, MPI_INT, rank2, 4,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    is_sorting_complete = odd_even_iteration(temp_buffer_1, temp_buffer_2,
                                             rank1, rank2, partition_size);
    // If even and not the first processor.
  } else if (!(node % 2) && node != 0) {
    rank1 = node;
    rank2 = node - 1;
    // Swap data with the paired processor.
    MPI_Sendrecv(temp_buffer_1, partition_size, MPI_INT, rank2, 4,
                 temp_buffer_2, partition_size, MPI_INT, rank2, 4,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    is_sorting_complete = odd_even_iteration(temp_buffer_1, temp_buffer_2,
                                             rank1, rank2, partition_size);
  }

  return is_sorting_complete;
}

/**
 * Gets the pairs of processors that will swap partitions and does some initial
 * sorting and processing.
 *
 * \param node The node's rank.
 * \param num_processors The number of processors.
 * \param temp_buffer_1 The buffer used to send.
 * \param temp_buffer_2 The buffer used to receive.
 * \param partition_size The size of the temporary buffers.
 */
void hypercube_compare_exchange(int node, int num_processors,
    int* temp_buffer_1, int* temp_buffer_2, int partition_size) {
  // Number of bits required to represent the ID of the process
  int d = log2(num_processors);
  // The rank of the processors that will swap buffers.
  int i, j, rank1, rank2;
  for (i = d - 1; i >= 0; i--) {
    rank1 = node;
    rank2 = get_hypercube_swap_node(rank1, i, num_processors);
    MPI_Sendrecv(temp_buffer_1, partition_size, MPI_INT, rank2, 1,
                 temp_buffer_2, partition_size, MPI_INT, rank2, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    compare_split(temp_buffer_1, temp_buffer_2, rank1, rank2, partition_size);
  }
}

/**
 * This function is continuously called while the subarrays are not fully
 * sorted. The subarrays will be combined, sorted, and redistributed similar to
 * compare_split.
 *
 * \param arr1 The array from the first processor.
 * \param arr2 The array from the second processor.
 * \param rank1 The rank of the first processor.
 * \param rank2 The rank of the second processor.
 * \param arr_size The size of each of the arrays.
 */
int odd_even_iteration(int* arr1, int* arr2, int rank1, int rank2,
                       int arr_size) {
  int i;
  int is_sorting_complete = 1;
  int* temp_arr = (int*)malloc(2 * arr_size * sizeof(int));
  // Merge the parameter arrays into the working space.
  for (i = 0; i < arr_size * 2; i++) {
    if (rank1 < rank2) {
      if (i < arr_size) {
        temp_arr[i] = arr1[i];
      } else {
        temp_arr[i] = arr2[i - arr_size];
      }
    } else {
      if (i < arr_size) {
        temp_arr[i] = arr2[i];
      } else {
        temp_arr[i] = arr1[i - arr_size];
      }
    }

    if (temp_arr[i] < temp_arr[i - 1] && i != 0) {
      is_sorting_complete = 0;
    }
  }

  shell_sort(temp_arr, arr_size * 2);

  if (rank1 < rank2) {
    for (i = 0; i < arr_size; i++) {
      arr1[i] = temp_arr[i];
    }
  } else {
    for (i = 0; i < arr_size; i++) {
      arr1[i] = temp_arr[i + arr_size];
    }
  }

  free(temp_arr);
  return is_sorting_complete;
}

/**
 * Performs the odd even transposition. Ensures that the partitions are fully
 * sorted and sorts them if they are not. Gathers all of the partitions and
 * returns them to the original array, completing the parallel shell sort.
 *
 * \param node The node's rank.
 * \param num_processors The number of processors.
 * \param temp_buffer_1 The buffer used to send.
 * \param temp_buffer_2 The buffer used to receive.
 * \param partition_size The size of the temporary buffers.
 */
void odd_even_transposition(int node, int num_processors, int* arr,
                            int* temp_buffer_1, int* temp_buffer_2,
                            int partition_size) {
  int is_sorting_complete = 0;
  int processor_sorted = 0;
  int counter = 1;
  int i, j;
  for (i = 0; i < num_processors; i++) {
    if (i % 2) {
      is_sorting_complete = odd_iteration(node, num_processors, temp_buffer_1,
                                          temp_buffer_2, partition_size);
    } else {
      is_sorting_complete = even_iteration(node, num_processors, temp_buffer_1,
                                           temp_buffer_2, partition_size);
    }

    if (node == MASTER) {
      if (i % 2) {
        counter = 1;
      }
      for (j = 1; j < num_processors; j++) {
        MPI_Recv(&processor_sorted, 1, MPI_INT, j, 0, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        counter += processor_sorted;
      }
    } else {
      // Let the master know if sorting is complete.
      MPI_Send(&is_sorting_complete, 1, MPI_INT, MASTER, 0, MPI_COMM_WORLD);
    }

    // Let the other processes know whether or not we have to keep going.
    MPI_Bcast(&counter, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    // The master has received the signal from every processor, so sorting is
    // complete.
    if (counter == num_processors * 2 - 1) {
      break;
    }
  }

  if (node == MASTER) {
    for (i = 0; i < partition_size; i++) {
      arr[i] = temp_buffer_1[i];
    }
    // Gather all of the subarrays.
    for (i = 1; i < num_processors; i++) {
      MPI_Recv(arr + (i * partition_size), partition_size, MPI_INT, i, 3,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
  } else {
    MPI_Send(temp_buffer_1, partition_size, MPI_INT, 0, 3, MPI_COMM_WORLD);
  }
}
