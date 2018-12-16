CC		=	/wsu/apps/gnu-4.4.7/openmpi/openmpi-1.8.8/bin/mpicc
CCLINK	        =	/wsu/apps/gnu-4.4.7/openmpi/openmpi-1.8.8/bin/mpicc
SHELL		=	/bin/sh

EXEC		=	matrix_addition

main:	matrix_addition.c
	$(CC) -o $(EXEC) $(EXEC).c

clean:
	/bin/rm -f jobscript.e* jobscript.o*
	/bin/rm -f john_shamoon_matrix_output.txt
	/bin/rm -f matrix_addition
