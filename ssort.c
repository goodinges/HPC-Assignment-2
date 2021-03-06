/* Parallel sample sort
 */
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>


static int compare(const void *a, const void *b)
{
  int *da = (int *)a;
  int *db = (int *)b;

  if (*da > *db)
    return 1;
  else if (*da < *db)
    return -1;
  else
    return 0;
}

int main( int argc, char *argv[])
{
  int rank;
  int i, j, N;
  int *vec;
  int sampleSize;
  int *sampleVec;
  int *allSamplesVec;
  int mpisize;
  int *splitters;
  int *scounts;
  int *sdispls;
  int *rcounts;
  int *rdispls;
  int *recvCounts;
  int recvCount;
  int *recvVec;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpisize);

  /* Number of random numbers per processor (this should be increased
   * for actual tests or could be passed in through the command line */
  N = 10000;

  vec = calloc(N, sizeof(int));
  sampleSize = N/mpisize;
  sampleVec = calloc(sampleSize, sizeof(int));
  /* seed random number generator differently on every core */
  srand((unsigned int) (rank + 393919));

  /* fill vector with random integers */
  for (i = 0; i < N; ++i) {
    vec[i] = rand();
  }
  printf("rank: %d, first entry: %d\n", rank, vec[0]);

  /* sort locally */
  qsort(vec, N, sizeof(int), compare);
  
  /* randomly sample s entries from vector or select local splitters,
   * i.e., every N/P-th entry of the sorted vector */
  for(i = 0; i < sampleSize; i++)
  {
	  sampleVec[i] = vec[rand()%N];
  }

  /* every processor communicates the selected entries
   * to the root processor; use for instance an MPI_Gather */
  if(rank==0)
  {
	  allSamplesVec = calloc(sampleSize*mpisize, sizeof(int));
  }
  MPI_Gather(sampleVec, sampleSize, MPI_INT, allSamplesVec, sampleSize, MPI_INT, 0, MPI_COMM_WORLD);

  /* root processor does a sort, determinates splitters that
   * split the data into P buckets of approximately the same size */
  splitters = calloc(mpisize - 1, sizeof(int));
  if(rank==0)
  {
	  qsort(allSamplesVec, sampleSize*mpisize, sizeof(int), compare);
	  for(i=0;i<sampleSize*mpisize;i++)
	  {
	  }
	  for(i = 0; i < mpisize - 1 ; i++)
	  {
		  splitters[i] = allSamplesVec[(i+1)*sampleSize];
	  }
  }

  /* root process broadcasts splitters */
  MPI_Bcast(splitters, mpisize - 1, MPI_INT, 0, MPI_COMM_WORLD);

  /* every processor uses the obtained splitters to decide
   * which integers need to be sent to which other processor (local bins) */
  scounts = calloc(mpisize, sizeof(int));
  sdispls = calloc(mpisize, sizeof(int));
  i = 0;
  for(j = 0; j < mpisize - 1 ; j++)
  {
	  sdispls[j] = i;
	  scounts[j] = 0;
	  while(vec[i] < splitters[j] && i < N)
	  {
		  scounts[j]++;
		  i++;
	  }
  }
  sdispls[j] = i;
  scounts[j] = N - i;

  /* send and receive: either you use MPI_AlltoallV, or
   * (and that might be easier), use an MPI_Alltoall to share
   * with every processor how many integers it should expect,
   * and then use MPI_Send and MPI_Recv to exchange the data */
  recvCounts = calloc(mpisize, sizeof(int));
  MPI_Alltoall(scounts, 1, MPI_INT, recvCounts, 1, MPI_INT, MPI_COMM_WORLD);
  recvCount = 0;
  rcounts = calloc(mpisize, sizeof(int));
  rdispls = calloc(mpisize, sizeof(int));
  for(i = 0; i < mpisize ; i++)
  {
	  rdispls[i] = recvCount;
	  recvCount += recvCounts[i];
	  rcounts[i] = recvCounts[i];
  }
  recvVec = calloc(recvCount, sizeof(int));
  MPI_Alltoallv(vec, scounts, sdispls, MPI_INT, recvVec, rcounts, rdispls, MPI_INT, MPI_COMM_WORLD);

  /* do a local sort */
  qsort(recvVec, recvCount, sizeof(int), compare);
  
  /* every processor writes its result to a file */
    FILE* fd = NULL;
    char filename[256];
    snprintf(filename, 256, "output%02d.txt", rank);
    fd = fopen(filename,"w+");

    if(NULL == fd)
    {
      printf("Error opening file \n");
      return 1;
    }

    for(i = 0; i < recvCount; i++)
    {
	    fprintf(fd, "%d\n", recvVec[i]);
    }
    fclose(fd);

  //free(vec);
  //free(sampleVec);
  if(rank==0)
  {
	  //free(allSamplesVec);
  }
  //free(splitters);
  /*free(scounts);
  free(sdispls);
  free(recvCounts);
  free(recvVec);
  free(rcounts);
  free(rdispls);*/
  //if(rank!=0)
  MPI_Finalize();
  return 0;
}
