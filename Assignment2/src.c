// Timing codes

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

double* data;
int N,myrank,size;
MPI_Request* request;
MPI_Status* status;

int main( int argc, char *argv[])
{
  double sTime, eTime, time, maxTime;

  MPI_Init(&argc, &argv);

  int data_size = atoi(argv[1]);
  N = data_size*128;
  
  //'data' for each process is the subdomain data that every process contains
  data = (double*)malloc(N*sizeof(double));

  //Random initialization of data
  for(int i=0;i<N;i++)
      data[i] = rand()%N;

  MPI_Comm_rank(MPI_COMM_WORLD, &myrank) ;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  

//  FILE* fptr;
//  //Opening data file for outputting execution time
//  if(!myrank)
//    fptr = fopen("data","a");

    default_time = 0;
    sTime = MPI_Wtime();
    MPI_Bcast(data,N,MPI_Double,0,MPI_COMM_WORLD);
    eTime = MPI_Wtime();
    default_time += eTime - sTime;

    optimized_time = 0;
    sTime = MPI_Wtime();
    eTime = MPI_Wtime();
    optimized_time += eTime - sTime;

  
    // obtain max time
    MPI_Reduce (&default_time, &max_default_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&optimized_time, &max_optimized_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (!myrank) 
    {
      printf("%lf\n",max_default_time);
      printf("%lf\n",max_optimized_time);
    }

//  if(!myrank)
//    fclose(fptr);


  MPI_Finalize();
  return 0;

}

