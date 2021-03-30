// Timing codes
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <math.h>
#include "mpi.h"

double* data,*intra_data,*recvdata,*data1,*data2,*data3,*recvdata1,*recvdata2,*recvdata3;
int* sendcounts,*recvcounts,*sdispls,*rdispls;
int N,myrank,size,groupid;
MPI_Request* request;
MPI_Status* status;

#define MAX_INTRASIZE 128

//Creating map of hostid vs groupid i.e. arr[2] = 1, means csews2 resides in group1 
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
    //Using last numbers of hostname as hostid e.g. csews30 will have hostid 30
    int hostid = atoi(hostname + 5);
    int groupid_map[93];
    create_groupidmap(groupid_map);
    int groupid = groupid_map[hostid];

    //intragroup is a subcommunicator containing all processes that reside on nodes of same node group
    MPI_Comm intragroup;
    //Using groupid as coloring criteria
    MPI_Comm_split (MPI_COMM_WORLD, groupid,myrank, &intragroup);

    int intragroup_rank;
    MPI_Comm_rank(intragroup, &intragroup_rank);
    
    //Intergroup subcommunicator consists of all processes with rank 0 in respective intragroup i.e. communicator of leader processes, where processes of rank 0 in respective intragroups are taken as leaders of the intragroup
    MPI_Comm intergroup;
    int color = 0;

    //Intergroup only defined for processes with intragroup_rank 0
    if(intragroup_rank != 0)
      color = MPI_UNDEFINED;
    MPI_Comm_split (MPI_COMM_WORLD, color ,myrank, &intergroup);
    int intergroup_rank;
    if(intragroup_rank == 0)
      MPI_Comm_rank(intergroup, &intergroup_rank);
  
    for(int i=0;i<n;i++){
    if(intragroup_rank == 0){
      //Broadcasting to all leader processes present in intergroup residing in different node groups
    MPI_Bcast(data,N,MPI_DOUBLE,0,intergroup);
    }
    //Broadcasting to all members of intragroup from leader process
    MPI_Bcast(data,N,MPI_DOUBLE,0,intragroup);
    }

    MPI_Comm_free(&intragroup);
    if(intragroup_rank == 0)
    MPI_Comm_free(&intergroup);
}

void default_reduce(int n){
  for(int i=0;i<n;i++){
    MPI_Reduce(data,recvdata,N,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  }
}

void optimized_reduce(int n){

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
    
    MPI_Comm intergroup;
    int color = 0;
    if(intragroup_rank != 0)
      color = MPI_UNDEFINED;
    MPI_Comm_split (MPI_COMM_WORLD, color ,myrank, &intergroup);
    int intergroup_rank;
    if(intragroup_rank == 0)
      MPI_Comm_rank(intergroup, &intergroup_rank);
  
    for(int i=0;i<n;i++){
      //First reducing in intragroup and collecting reduced values at the leader of intragroup
    MPI_Reduce(data,intra_data,N,MPI_DOUBLE,MPI_SUM,0,intragroup);
    if(intragroup_rank == 0){

      //Reducing values collected at leader processes and collecting at the global leader(Rank 0 of intergroup)
    MPI_Reduce(intra_data,recvdata,N,MPI_DOUBLE,MPI_SUM,0,intergroup);
    }
    }

    MPI_Comm_free(&intragroup);
    if(intragroup_rank == 0)
    MPI_Comm_free(&intergroup);
}


void default_gather(int n){
  for(int i=0;i<n;i++){
    MPI_Gather(data,N,MPI_DOUBLE,recvdata,N,MPI_DOUBLE,0,MPI_COMM_WORLD);
  }
}

void optimized_gather(int n){
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
    
    MPI_Comm intergroup;
    int color = 0;
    if(intragroup_rank != 0)
      color = MPI_UNDEFINED;
    MPI_Comm_split (MPI_COMM_WORLD, color ,myrank, &intergroup);
    int intergroup_rank;
    if(intragroup_rank == 0)
      MPI_Comm_rank(intergroup, &intergroup_rank);
  
    for(int i=0;i<n;i++){
      //Appending rank at the end of data to sort the gathered values in the end by rank
      data[N] = myrank;
      //Gathering values at the leader of intragroup from the members
      MPI_Gather(data,(N+1),MPI_DOUBLE,intra_data,(N+1),MPI_DOUBLE,0,intragroup);
    if(intragroup_rank == 0){
      int intragroupsize;
      MPI_Comm_size(intragroup,&intragroupsize);
      int rcounts[10];
      //Collecting intragroupsize values from all leaders to the global leader, used in gatherv later
      MPI_Gather(&intragroupsize,1,MPI_INT,rcounts,1,MPI_INT,0,intergroup);
      int intergroupsize;
      MPI_Comm_size(intergroup,&intergroupsize);
      int displs[10];
      displs[0]  = 0;

      //Sendcount from a leader will be equal to intragroupsize*(N+1) and so will be the receivecount for the process collecting data
      for(int i=0;i<intergroupsize;i++)
        rcounts[i] *= (N+1);
      for(int i=1;i<intergroupsize;i++)
        displs[i] = displs[i-1] + rcounts[i-1];

      //Using gatherv for collecting data from all leaders to the global leader since intragroupsize for different leaders can be different
      MPI_Gatherv(intra_data,(N+1)*intragroupsize,MPI_DOUBLE,recvdata1,rcounts,displs,MPI_DOUBLE,0,intergroup);

      //Using the rank appended at the end of data at sendtime to sort the values by rank
      for(int i=0;i<size;i++){
        int currank = recvdata1[(N+1)*i + N];
        for(int j=0; j<N;j++){
          recvdata[currank*N+j] = recvdata1[(N+1)*i+j];
        }
      }
    }
    }

    MPI_Comm_free(&intragroup);
    if(intragroup_rank == 0)
    MPI_Comm_free(&intergroup);

}

void default_alltoallv(int n){

  for(int i=0;i<n;i++){

  MPI_Alltoall(sendcounts,1,MPI_INT,recvcounts,1,MPI_INT,MPI_COMM_WORLD);
  rdispls[0] = 0;
  for(int i=1;i<size;i++){
    rdispls[i] = rdispls[i-1] + recvcounts[i-1];
  }
  MPI_Alltoallv(data,sendcounts,sdispls,MPI_DOUBLE,recvdata,recvcounts,rdispls,MPI_DOUBLE,MPI_COMM_WORLD);
  }

}
void optimized_alltoallv(int n){
    char hostname[64];
    int len;
    MPI_Get_processor_name(hostname, &len);
    int hostid = atoi(hostname + 5);
    int groupid_map[93];
    create_groupidmap(groupid_map);
    int groupid = groupid_map[hostid];

    MPI_Comm intragroup;
    MPI_Comm_split (MPI_COMM_WORLD, groupid,myrank, &intragroup);
    int intragroup_rank,intragroup_size;
    MPI_Comm_rank(intragroup, &intragroup_rank);
    MPI_Comm_size(intragroup, &intragroup_size);
    
    MPI_Comm intergroup;
    int color = 0;
    if(intragroup_rank != 0)
      color = MPI_UNDEFINED;
    MPI_Comm_split (MPI_COMM_WORLD, color ,myrank, &intergroup);
    int intergroup_rank,intergroup_size;
    if(intragroup_rank == 0){
    MPI_Comm_rank(intergroup, &intergroup_rank);
    MPI_Comm_size(intergroup, &intergroup_size);
    }

    for(int i=0;i<n;i++){

    long pos1 = 0,pos2 = 0;

    //Padding the data
    data1[pos2++] = myrank;
    for(int i=0;i<size;i++){
      int cursize = sendcounts[i];
      pos2 = 1 + i*(N+2);
      data1[pos2++] = i;
      data1[pos2++] = cursize;
      for(int j=pos1;j<pos1+cursize;j++){
        data1[pos2++]=data[j];
      }
      pos1 = pos1+cursize;
    }

    MPI_Gather(data1,1+(N+2)*size,MPI_DOUBLE,data2,1+(N+2)*size,MPI_DOUBLE,0,intragroup);
    if(intragroup_rank == 0){
      int ranks[MAX_INTRASIZE];
      ranks[0] = intragroup_size;
      for(int i=0;i<intragroup_size;i++){
        ranks[i+1] = data2[(1+(N+2)*size)*i];
      }

      int all_ranks[MAX_INTRASIZE*intergroup_size];
      MPI_Allgather(ranks,MAX_INTRASIZE,MPI_INT,all_ranks,MAX_INTRASIZE,MPI_INT,intergroup);
      pos2 = 0;
      int intergroup_sendcounts[intergroup_size];
      int intergroup_recvcounts[intergroup_size];
      for(int i=0;i<intergroup_size;i++){
        int intrasize = all_ranks[MAX_INTRASIZE*i];
        for(int j=MAX_INTRASIZE*i+1;j<=MAX_INTRASIZE*i+intrasize;j++){
          int currank = all_ranks[j];
          for(int k=0;k<intragroup_size;k++){
            pos1 = (1+(N+2)*size)*k+1+(N+2)*currank;
            for(int l=0;l<N+2;l++){
              data3[pos2++] = data2[pos1+l];
            }
          }
        }
        intergroup_sendcounts[i] = intrasize*intragroup_size*(N+2);
        intergroup_recvcounts[i] = intrasize*intragroup_size*(N+2);
      }
      int intergroup_sdispls[intergroup_size];
      int intergroup_rdispls[intergroup_size];
      intergroup_sdispls[0] = 0;
      intergroup_rdispls[0] = 0;
      for(int i=1;i<intergroup_size;i++){
        intergroup_sdispls[i] = intergroup_sdispls[i-1] + intergroup_sendcounts[i-1];
        intergroup_rdispls[i] = intergroup_rdispls[i-1] + intergroup_recvcounts[i-1];
      }
      MPI_Alltoallv(data3,intergroup_sendcounts,intergroup_sdispls,MPI_DOUBLE,recvdata3,intergroup_recvcounts,intergroup_rdispls,MPI_DOUBLE,intergroup);

      pos2 = 0;
      for(int i=0;i<intragroup_size;i++){
        int rank = ranks[i];
        for(int j=0;j<intergroup_size;j++){
          int nblocks = all_ranks[MAX_INTRASIZE*j];
          pos1 = intergroup_rdispls[j]+nblocks*i*(N+2);
          for(int k=0;k<nblocks*(N+2);k++){
            recvdata2[pos2++] = recvdata3[pos1+k];
          }
          }
        }
    }
      MPI_Scatter(recvdata2,size*(N+2),MPI_DOUBLE,recvdata1,size*(N+2),MPI_DOUBLE,0,intragroup);
      pos1 = 0, pos2 = 0;
      for(int i=0;i<size;i++){
        pos1 = (N+2)*i;
        recvcounts[i]= recvdata1[pos1++];
        for(int j=0;j<recvcounts[i];j++)
          recvdata[pos2++] = recvdata1[pos1+j];
      }

      int pos = 0;

    }
    MPI_Comm_free(&intragroup);
    if(intragroup_rank == 0)
    MPI_Comm_free(&intergroup);

}

int main( int argc, char *argv[])
{
  double sTime, eTime, default_bcast_time = 0,optimized_bcast_time = 0,default_reduce_time = 0,optimized_reduce_time = 0,default_gather_time = 0,optimized_gather_time = 0,max_default_bcast_time = 0,max_default_reduce_time = 0,max_optimized_bcast_time = 0,max_optimized_reduce_time = 0,max_default_gather_time = 0,max_optimized_gather_time = 0,default_alltoallv_time=0,optimized_alltoallv_time=0,max_default_alltoallv_time=0,max_optimized_alltoallv_time = 0;
  int len;

  MPI_Init(&argc, &argv);

  int data_size = atoi(argv[1]);
  N = data_size*128;
  
  //'data' for each process is the subdomain data that every process contains

  //Random initialization of data

  MPI_Comm_rank(MPI_COMM_WORLD, &myrank) ;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int coreid = sched_getcpu();
  data = (double*)malloc((N+1)*size*sizeof(double));
  data1 = (double*)malloc((1+(N+2)*size)*sizeof(double));
  data2 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  data3 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  recvdata3 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  recvdata2 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  recvdata1 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  intra_data = (double*)malloc(N*MAX_INTRASIZE*sizeof(double));
  recvdata = (double*)malloc((N+1)*(size+1)*sizeof(double));

  sendcounts = (int*)malloc(size*sizeof(int));
  sdispls = (int*)malloc(size*sizeof(int));
  for(int i=0;i<size;i++){
    sendcounts[i] = 1;
  }
  sdispls[0] = 0;
  for(int i=1;i<size;i++){
    sdispls[i] = sdispls[i-1] + sendcounts[i-1];
  }
  recvcounts = (int*)malloc(size*sizeof(int));
  rdispls = (int*)malloc(size*sizeof(int));
  for(int i=0;i<N;i++)
      data[i] = rand()%N;


  FILE* fptr;
  //Opening data file for outputting execution time
  if(!myrank)
    fptr = fopen("data1","a");




//  MPI_Group g_group;
//  MPI_Comm_group (MPI_COMM_WORLD, &g_group);
//
//  MPI_Group new_group;
//  MPI_Group_incl (g_group, ranks_size, ranks, &new_group);

  

//  FILE* fptr;
//  //Opening data file for outputting execution time
//  if(!myrank)
//    fptr = fopen("data","a");

    sTime = MPI_Wtime();
    default_bcast(5);
    eTime = MPI_Wtime();
    default_bcast_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    optimized_bcast(5);
    eTime = MPI_Wtime();
    optimized_bcast_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    default_reduce(5);
    eTime = MPI_Wtime();
    default_reduce_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    optimized_reduce(5);
    eTime = MPI_Wtime();
    optimized_reduce_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    default_gather(5);
    eTime = MPI_Wtime();
    default_gather_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    optimized_gather(5);
    eTime = MPI_Wtime();
    optimized_gather_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    default_alltoallv(5);
    eTime = MPI_Wtime();
    default_alltoallv_time = (eTime - sTime)/5;

    sTime = MPI_Wtime();
    optimized_alltoallv(5);
    eTime = MPI_Wtime();
    optimized_alltoallv_time = (eTime - sTime)/5;
  
    // obtain max time
    MPI_Reduce (&default_bcast_time, &max_default_bcast_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&optimized_bcast_time, &max_optimized_bcast_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&default_reduce_time, &max_default_reduce_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&optimized_reduce_time, &max_optimized_reduce_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Reduce (&default_gather_time, &max_default_gather_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&optimized_gather_time, &max_optimized_gather_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&default_alltoallv_time, &max_default_alltoallv_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce (&optimized_alltoallv_time, &max_optimized_alltoallv_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (!myrank) 
    {
      printf("bcast default %lf optimized %lf speedup=%lf\n",max_default_bcast_time,max_optimized_bcast_time,max_default_bcast_time/max_optimized_bcast_time);
      printf("reduce default %lf optimized %lf speedup=%lf\n",max_default_reduce_time,max_optimized_reduce_time,max_default_reduce_time/max_optimized_reduce_time);
      printf("gather default %lf optimized %lf speedup=%lf\n",max_default_gather_time,max_optimized_gather_time,max_default_gather_time/max_optimized_gather_time);
      printf("alltoallv default %lf optimized %lf speedup=%lf\n",max_default_alltoallv_time,max_optimized_alltoallv_time,max_default_alltoallv_time/max_optimized_alltoallv_time);
    fprintf (fptr,"%lf\n", max_default_bcast_time);
    fprintf (fptr,"%lf\n", max_optimized_bcast_time);
    fprintf (fptr,"%lf\n", max_default_reduce_time);
    fprintf (fptr,"%lf\n", max_optimized_reduce_time);
    fprintf (fptr,"%lf\n", max_default_gather_time);
    fprintf (fptr,"%lf\n", max_optimized_gather_time);
    fprintf (fptr,"%lf\n", max_default_alltoallv_time);
    fprintf (fptr,"%lf\n", max_optimized_alltoallv_time);
    }

  if(!myrank)
    fclose(fptr);


  MPI_Finalize();
  return 0;

}

