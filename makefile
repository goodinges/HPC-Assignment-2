a: ssort.c
	mpicc mpi_solved1.c -lrt -lm -o mpi_solved1
	mpicc mpi_solved2.c -lrt -lm -o mpi_solved2
	mpicc mpi_solved3.c -lrt -lm -o mpi_solved3
	mpicc mpi_solved4.c -lrt -lm -o mpi_solved4
	mpicc mpi_solved5.c -lrt -lm -o mpi_solved5
	mpicc mpi_solved6.c -lrt -lm -o mpi_solved6
	mpicc mpi_solved7.c -lrt -lm -o mpi_solved7
	mpicc ssort.c -lrt -lm -o ssort
