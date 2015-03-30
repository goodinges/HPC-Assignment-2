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
  N = 10;

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
	  sampleVec[i] = vec[i*mpisize];
		printf("%d->sampleVec[%d]:%d\n", rank, i, sampleVec[i]);
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
  if(rank==0)
  {
	  qsort(allSamplesVec, sampleSize*mpisize, sizeof(int), compare);
	for(i=0;i<sampleSize*mpisize;i++)
	{
		printf("%d->allSamplesVec[%d]:%d\n", rank, i, allSamplesVec[i]);
	}
	  splitters = calloc(mpisize - 1, sizeof(int));
	  for(i = 1; i < mpisize ; i++)
	  {
		  splitters[i] = allSamplesVec[i*sampleSize];
		printf("%d->splitters[%d]:%d\n", rank, i, splitters[i]);
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
	  while(vec[i] < splitters[j])
	  {
		  scounts[j]++;
		  i++;
	  }
  }
  //j++;
  sdispls[j] = i;
  scounts[j] = N - i;
	for(i=0;i<mpisize;i++)
	{
		printf("%d->scount[%d]:%d\n", rank, i, scounts[i]);
	}
  //printf("sdispls=%d\n", sdispls[0]);
  //printf("scounts=%d\n", scounts[0]);

  /* send and receive: either you use MPI_AlltoallV, or
   * (and that might be easier), use an MPI_Alltoall to share
   * with every processor how many integers it should expect,
   * and then use MPI_Send and MPI_Recv to exchange the data */
  recvCounts = calloc(mpisize, sizeof(int));
  MPI_Alltoall(scounts, mpisize, MPI_INT, recvCounts, mpisize, MPI_INT, MPI_COMM_WORLD);
  recvCount = 0;
  rcounts = calloc(mpisize, sizeof(int));
  for(i = 0; i < mpisize ; i++)
  {
	  recvCount += recvCounts[i];
	  rcounts[i] = recvCounts[i];
  }
  printf("\nrecvCount=%d\n", recvCount);
  recvVec = calloc(recvCount, sizeof(int));
  rdispls = calloc(mpisize, sizeof(int));
  MPI_Alltoallv(vec, scounts, sdispls, MPI_INT, recvVec, rcounts, rdispls, MPI_INT, MPI_COMM_WORLD);
  printf("\n---\n");
	//for(i=0;i<recvCount;i++)
	//{
	//	printf("%d\t", recvVec[i]);
	//}
	//printf("\n");
  /* do a local sort */
  qsort(recvVec, recvCount, sizeof(int), compare);
printf("\n%d***\n",rank);

  /* every processor writes its result to a file */
  printf("Rank %d: ", rank);
  for(i = 0; i < recvCount; i++)
  {
	  printf("%d ", recvVec[i]);
  }
  printf("\n");
printf("\n%d***\n",rank);

  free(vec);
  free(sampleVec);
  if(rank==0)
  {
	  free(allSamplesVec);
  }
  free(splitters);
  free(scounts);
  free(sdispls);
  free(recvCounts);
  free(recvVec);
  free(rcounts);
  free(rdispls);
  MPI_Finalize();
  return 0;
}
