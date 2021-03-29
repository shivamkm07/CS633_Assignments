// Timing codes
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <math.h>
#include "mpi.h"

double* data;
int N,myrank,size;
MPI_Request* request;
MPI_Status* status;


void create_groupidmap(int* arr){
	arr[0] = -1;
	for(int i = 1; i<=16; i++){
	    if (i==13)
	        continue;
	    arr[i] = 0;
	}
	
	arr[13] = 1;
	arr[31] = 0;
	
	for(int i = 17; i<=30; i++){
	    arr[i] = 1;
	}
	
	arr[32] = 1;
	
	for(int i = 33; i<=44; i++){
	    arr[i] = 2;
	}
	
	arr[46] = 2;
	arr[45] = 3;
	    
	for(int i = 47; i<=61; i++){
	    arr[i] = 3;
	}
	
	for(int i = 62; i<=78; i++){
	    arr[i] = 4;
	}
	
	for(int i = 79; i<=92; i++){
	    arr[i] = 5;
	}
}

void default_bcast(int n){

  for(int i = 0;i <n;i++){
    MPI_Bcast(data,N,MPI_DOUBLE,0,MPI_COMM_WORLD);
  }
}

void optimized_bcast(int n){
  char hostname[64];
  int len;
  MPI_Get_processor_name(hostname, &len);
  int hostid = atoi(hostname + 5);
  int groupid_map[93];
  create_groupidmap(groupid_map);
  int groupid = groupid_map[hostid];

  
    MPI_Comm intragroup;
    MPI_Comm_split (MPI_COMM_WORLD, groupid,myrank, &intragroup);
  
    int intragroup_rank;
    MPI_Comm_rank(intragroup, &intragroup_rank);

//    printf("myrank %d hostid %d groupid %d intragroup_rank %d\n",myrank,hostid,groupid,intragroup_rank);
    
    MPI_Comm intergroup;
    int color = 0;
    if(intragroup_rank != 0)
      color = MPI_UNDEFINED;
    MPI_Comm_split (MPI_COMM_WORLD, color ,myrank, &intergroup);
    int intergroup_rank;
    if(intragroup_rank == 0)
      MPI_Comm_rank(intergroup, &intergroup_rank);
  
  
    for(int i=0;i<n;i++){
    if(intragroup_rank == 0){
    MPI_Bcast(data,N,MPI_DOUBLE,0,intergroup);
    }
    MPI_Bcast(data,N,MPI_DOUBLE,0,intragroup);
    }


}
int main( int argc, char *argv[])
{
  double sTime, eTime, default_time,optimized_time,max_default_time,max_optimized_time;
  int len;

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
  int coreid = sched_getcpu();






//  MPI_Group g_group;
//  MPI_Comm_group (MPI_COMM_WORLD, &g_group);
//
//  MPI_Group new_group;
//  MPI_Group_incl (g_group, ranks_size, ranks, &new_group);

  

//  FILE* fptr;
//  //Opening data file for outputting execution time
//  if(!myrank)
//    fptr = fopen("data","a");

    default_time = 0;
    sTime = MPI_Wtime();
    default_bcast(5);
    eTime = MPI_Wtime();
    default_time = (eTime - sTime)/5;

    optimized_time = 0;
    sTime = MPI_Wtime();
    optimized_bcast(5);
    eTime = MPI_Wtime();
    optimized_time = (eTime - sTime)/5;

  
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

