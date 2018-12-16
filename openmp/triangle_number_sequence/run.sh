setenv OMP_NUM_THREADS 1
echo "NUM_THREADS: $OMP_NUM_THREADS"
gcc -fopenmp triangle_number_sequence.c
./a.out

setenv OMP_NUM_THREADS 2
echo "NUM_THREADS: $OMP_NUM_THREADS"
gcc -fopenmp triangle_number_sequence.c
./a.out

setenv OMP_NUM_THREADS 4
echo "NUM_THREADS: $OMP_NUM_THREADS"
gcc -fopenmp triangle_number_sequence.c
./a.out

setenv OMP_NUM_THREADS 8
echo "NUM_THREADS: $OMP_NUM_THREADS"
gcc -fopenmp triangle_number_sequence.c
./a.out
