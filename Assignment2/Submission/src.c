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

//Default MPI_Bcast
void default_bcast(int n){

  for(int i = 0;i <n;i++){
    MPI_Bcast(data,N,MPI_DOUBLE,0,MPI_COMM_WORLD);
  }
}

//Optimized MPI_BCast
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

//Default MPI_reduce
void default_reduce(int n){
  for(int i=0;i<n;i++){
    MPI_Reduce(data,recvdata,N,MPI_DOUBLE,MPI_SUM,0,MPI_COMM_WORLD);
  }
}

//Optimized MPI_Reduce
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

//Default MPI_Gather
void default_gather(int n){
  for(int i=0;i<n;i++){
    MPI_Gather(data,N,MPI_DOUBLE,recvdata,N,MPI_DOUBLE,0,MPI_COMM_WORLD);
  }
}

//Optimized MPI_Gather
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

//Default MPI_Alltoallv
void default_alltoallv(int n){

  for(int i=0;i<n;i++){

    //Populating recvcounts of the processes through MPI_alltoall
  MPI_Alltoall(sendcounts,1,MPI_INT,recvcounts,1,MPI_INT,MPI_COMM_WORLD);
  rdispls[0] = 0;
  for(int i=1;i<size;i++){
    rdispls[i] = rdispls[i-1] + recvcounts[i-1];
  }
  MPI_Alltoallv(data,sendcounts,sdispls,MPI_DOUBLE,recvdata,recvcounts,rdispls,MPI_DOUBLE,MPI_COMM_WORLD);
  }

}

//Optimized implementation of alltoallv(version 1) using padding to make chunksize constant to reduce no of collective calls
void optimized_alltoallv_v1(int n){
    char hostname[64];
    int len;
    MPI_Get_processor_name(hostname, &len);
    int hostid = atoi(hostname + 5);
    int groupid_map[93];
    create_groupidmap(groupid_map);
    int groupid = groupid_map[hostid];

    //Creating intragroup of processes on same node group
    MPI_Comm intragroup;
    MPI_Comm_split (MPI_COMM_WORLD, groupid,myrank, &intragroup);
    int intragroup_rank,intragroup_size;
    MPI_Comm_rank(intragroup, &intragroup_rank);
    MPI_Comm_size(intragroup, &intragroup_size);
    
    //creating inntergroup of leader processes residing on different node groups
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

    //Padding the data so that each data chunk to be sent to each of the processes consist of N+2 Doubles, N is the maximum number of doubles to be sent to each of the processes, and 2 doubles for storing rank of process and actual sendcount of that chunk respectively.
    //Also myrank appended at the starting, so the total size of the data array is 1 + (N+2)*size doubles
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

    //Collecting the data,sendcounts and rank at leader process of intragroup
    MPI_Gather(data1,1+(N+2)*size,MPI_DOUBLE,data2,1+(N+2)*size,MPI_DOUBLE,0,intragroup);


    if(intragroup_rank == 0){

      //ranks[0] is the intragroupsize and rest intragroup_size numbers are the ranks of processes prsent in intragroup
      int ranks[MAX_INTRASIZE];
      ranks[0] = intragroup_size;
      //Taking out myranks sent along with data
      for(int i=0;i<intragroup_size;i++){
        ranks[i+1] = data2[(1+(N+2)*size)*i];
      }

      //All_ranks contain intragroupsize and ranks of all processes, calculated using allgather on ranks array 
      int all_ranks[MAX_INTRASIZE*intergroup_size];
      MPI_Allgather(ranks,MAX_INTRASIZE,MPI_INT,all_ranks,MAX_INTRASIZE,MPI_INT,intergroup);

      pos2 = 0;
      int intergroup_sendcounts[intergroup_size];
      int intergroup_recvcounts[intergroup_size];

      //Sorting the chunks by the ranks of processes where they have to be sent, ranks of same group grouped together
      for(int i=0;i<intergroup_size;i++){
        int intrasize = all_ranks[MAX_INTRASIZE*i];
        for(int j=MAX_INTRASIZE*i+1;j<=MAX_INTRASIZE*i+intrasize;j++){
          int currank = all_ranks[j];
          //Collecting all data chunk to be sent to rank currank
          for(int k=0;k<intragroup_size;k++){
            pos1 = (1+(N+2)*size)*k+1+(N+2)*currank;
            for(int l=0;l<N+2;l++){
              data3[pos2++] = data2[pos1+l];
            }
          }
        }
        //intergroup_sendcounts consists of chunksize to be sent to each of the leader process in intergroup i.e. count of chunks to be sent ot each group
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

      //Collecting data from all groups to the receiving group
      MPI_Alltoallv(data3,intergroup_sendcounts,intergroup_sdispls,MPI_DOUBLE,recvdata3,intergroup_recvcounts,intergroup_rdispls,MPI_DOUBLE,intergroup);

      pos2 = 0;
      //Sorting the data again so that data to be received to by each process is grouped together
      for(int i=0;i<intragroup_size;i++){
        int rank = ranks[i];
        //Collecting all data to be sent to rank "rank"
        for(int j=0;j<intergroup_size;j++){
          int nblocks = all_ranks[MAX_INTRASIZE*j];
          pos1 = intergroup_rdispls[j]+nblocks*i*(N+2);
          for(int k=0;k<nblocks*(N+2);k++){
            recvdata2[pos2++] = recvdata3[pos1+k];
          }
          }
        }
    }

    //Scattering data so that each process gets it's each own chunk
      MPI_Scatter(recvdata2,size*(N+2),MPI_DOUBLE,recvdata1,size*(N+2),MPI_DOUBLE,0,intragroup);


      //Finally removing padding and extra space to get recvdata and recvcounts 
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

// Optimized implementation of alltoallv(version2)
void optimized_alltoallv_v2(int n){
    char hostname[64];
    int len;
    MPI_Get_processor_name(hostname, &len);
    int hostid = atoi(hostname + 5);
    int groupid_map[93];
    create_groupidmap(groupid_map);
    int groupid = groupid_map[hostid];

    //Creating intargroup of all processes residing on same node group
    MPI_Comm intragroup;
    MPI_Comm_split (MPI_COMM_WORLD, groupid,myrank, &intragroup);
    int intragroup_rank,intragroup_size;
    MPI_Comm_rank(intragroup, &intragroup_rank);
    MPI_Comm_size(intragroup, &intragroup_size);
    
    //creaing intergroup of all leader processes residing on different node groups
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

    //Appending myrank at the end of sendcounts to reduce the number of collective calls
    sendcounts[size] = myrank;


    //intra_counts(at rank 0 of intragroup) contains sendcounts of all processes in the intragroup
    int intra_counts[(size+1)*intragroup_size];
    int intra_displs[size*intragroup_size];
    MPI_Gather(sendcounts,size+1,MPI_INT,intra_counts,size+1,MPI_INT,0,intragroup);


    intra_displs[0] = 0;
    for(int i=0;i<intragroup_size;i++){
      for(int j=0;j<size;j++){
        if(i==0 && j==0)continue;
        intra_displs[size*i+j] = intra_displs[size*i+j-1] + intra_counts[(size+1)*i+j-1];
      }
    }

    //intra_rcounts[i] contain the sum of sendcounts of ith rank process in intragroup,intra_rdispls corresponding displacements
    int intra_rcounts[intragroup_size],intra_rdispls[intragroup_size];

    for(int i=0;i<intragroup_size;i++){
      int sum = 0;
      for(int j=(size+1)*i;j<(size+1)*i+size;j++)
        sum += intra_counts[j];
      intra_rcounts[i] = sum;
      if(i == 0)
        intra_rdispls[i] = 0;
      else
        intra_rdispls[i] = intra_rdispls[i-1] + intra_rcounts[i-1];
    }

    //sum_sendcount at each process contain the total chunksize to be sent by that process
    int sum_sendcount = 0;
    for(int i=0;i<size;i++)
      sum_sendcount += sendcounts[i];

    //Collecting all data to be sent at leader process of intragroup
    MPI_Gatherv(data,sum_sendcount,MPI_DOUBLE,data1,intra_rcounts,intra_rdispls,MPI_DOUBLE,0,intragroup);

    int finalcount_myranks[intragroup_size];
    int finalcount_myranks_displs[intragroup_size];
    if(intragroup_rank == 0){

      //ranks[0] is the intragroupsize and next intragroup_size numbers are the ranks of processes prsent in intragroup
      int ranks[MAX_INTRASIZE];
      ranks[0] = intragroup_size;
      //Taking out myranks sent along with sendcounts in intra_counts
      for(int i=0;i<intragroup_size;i++){
        ranks[i+1] = intra_counts[(size+1)*i+size];
      }

      //All_ranks contain intragroupsize and ranks of all processes, calculated using allgather on ranks array 
      int all_ranks[MAX_INTRASIZE*intergroup_size];
      MPI_Allgather(ranks,MAX_INTRASIZE,MPI_INT,all_ranks,MAX_INTRASIZE,MPI_INT,intergroup);

      pos2 = 0;

      int intergroup_sendcounts[intergroup_size];
      int intergroup_recvcounts[intergroup_size];

      //intra_totsize contains the counts of data(doubles) to be sent to different processes from the current intragroup(leader)
      int intra_totsize[size];
      //count_intergroup_scounts[i] contains number of processes present in ith group i.e. number of process chunks to be sent to ith group where a process chunk consists of all data to be sent to that process by current group
      //count_intergroup_rcounts[i] contains number of process chunks received by ith group i.e. intragroup_size
      int count_intergroup_rcounts[intergroup_size],count_intergroup_scounts[intergroup_size];
      int count_intergroup_rdispls[intergroup_size],count_intergroup_sdispls[intergroup_size];
      int cur = 0;

      //Grouping the data by the processes where they are to be sent i.e. in process chunks, processes of same group grouped together
      for(int i=0;i<intergroup_size;i++){
        //intrasize is the number of processes present in ith group
        int intrasize = all_ranks[MAX_INTRASIZE*i];
        count_intergroup_scounts[i] = intrasize;
        count_intergroup_rcounts[i] = intragroup_size;
        for(int j=MAX_INTRASIZE*i+1;j<=MAX_INTRASIZE*i+intrasize;j++){
          int currank = all_ranks[j];
          int sum = 0;
          for(int k=0;k<intragroup_size;k++){
            pos1 = intra_displs[size*k + currank];
            int num = intra_counts[(size+1)*k + currank];
            sum += num;
//            pos1 = (1+(N+2)*size)*k+1+(N+2)*currank;
            for(int l=0;l<num;l++){
              data2[pos2++] = data1[pos1+l];
            }
          }
          intra_totsize[cur++] = sum;
        }
      }

      count_intergroup_sdispls[0] = 0;
      count_intergroup_rdispls[0] = 0;
      for(int i=1;i<intergroup_size;i++){
        count_intergroup_sdispls[i] = count_intergroup_sdispls[i-1] + count_intergroup_scounts[i-1];
        count_intergroup_rdispls[i] = count_intergroup_rdispls[i-1] + count_intergroup_rcounts[i-1];
      }

      //inter_myrank_totsize contains the counts of data(doubles) receieved from different groups to current intragroup, each intragroup_size chunk contains the data received for different processes in current group from a given group
      int inter_myrank_totsize[intragroup_size*intergroup_size];
      MPI_Alltoallv(intra_totsize, count_intergroup_scounts,count_intergroup_sdispls,MPI_INT,inter_myrank_totsize,count_intergroup_rcounts,count_intergroup_rdispls,MPI_INT,intergroup);

      for(int i=0;i<intergroup_size;i++){
        int pos = count_intergroup_sdispls[i];
        int num = count_intergroup_scounts[i];
        int sum = 0;
        //Summing counts for ith intrasize block of intra_totsize to get the total sendcount to be sent to ith group
        for(int j=0;j<num;j++){
          sum += intra_totsize[pos+j];
        }
        intergroup_sendcounts[i] = sum;
        sum = 0;
        pos = count_intergroup_rdispls[i];
        num = count_intergroup_rcounts[i];
        //Summing count for ith intragroup_size block of inter_myrank_totsize to get the total recievecount to be sent to ith group
        for(int j=0;j<num;j++){
          sum += inter_myrank_totsize[pos+j];
        }
        intergroup_recvcounts[i] = sum;
      }

      int intergroup_sdispls[intergroup_size];
      int intergroup_rdispls[intergroup_size];
      intergroup_sdispls[0] = 0;
      intergroup_rdispls[0] = 0;
      for(int i=1;i<intergroup_size;i++){
        intergroup_sdispls[i] = intergroup_sdispls[i-1] + intergroup_sendcounts[i-1];
        intergroup_rdispls[i] = intergroup_rdispls[i-1] + intergroup_recvcounts[i-1];
      }

      //Sending and collecting data across all groups, intergroup_sendcounts array containing data sent to each group, intergroup_recvcounts containing data received from each group
      MPI_Alltoallv(data2,intergroup_sendcounts,intergroup_sdispls,MPI_DOUBLE,recvdata2,intergroup_recvcounts,intergroup_rdispls,MPI_DOUBLE,intergroup);

      int inter_myrank_totsize_displs[intragroup_size*intergroup_size];
      inter_myrank_totsize_displs[0] = 0;
      for(int i=1;i<intragroup_size*intergroup_size;i++)
        inter_myrank_totsize_displs[i] = inter_myrank_totsize_displs[i-1] + inter_myrank_totsize[i-1];

      //Collecting data of each process together to form final process chunk to be sent to each processs
      pos2 = 0;
      for(int i=0;i<intragroup_size;i++){
        int rank = ranks[i];
        int sum = 0;
        for(int j=0;j<intergroup_size;j++){
          int pos = inter_myrank_totsize_displs[intragroup_size*j + i];
          int num = inter_myrank_totsize[intragroup_size*j + i];
          sum += num;
          for(int k=0;k<num;k++){
            recvdata1[pos2++] = recvdata2[pos1+k];
          }
          }
        //finalcount_myranks[i] contains the final count of data to be sent to process with rank i in current intragroup
        finalcount_myranks[i] = sum;
        }
    finalcount_myranks_displs[0] = 0;
    for(int i=1;i<intragroup_size;i++){
      finalcount_myranks_displs[i] = finalcount_myranks_displs[i-1] + finalcount_myranks[i-1];
    }
    }
    int recvcount = 0;
    //Scattering finalcount_myranks among intragroup so that each process gets recvcount
    MPI_Scatter(finalcount_myranks,1,MPI_INT,&recvcount,1,MPI_INT,0,intragroup);
    //Finally scattering the data so that each process gets it's data in recvdata array
    MPI_Scatterv(recvdata1,finalcount_myranks,finalcount_myranks_displs,MPI_DOUBLE,recvdata,recvcount,MPI_DOUBLE,0,intragroup);
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
  //N is the number of doubles, which is equal to D*1024/8 = D*128
  N = data_size*128;
  

  MPI_Comm_rank(MPI_COMM_WORLD, &myrank) ;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  int coreid = sched_getcpu();
  //'data' for each process is the subdomain data that every process contains
  data = (double*)malloc((N+1)*size*sizeof(double));

  //Allocating other temporray data memory required in several functions
  data1 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  data2 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  data3 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  recvdata3 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  recvdata2 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  recvdata1 = (double*)malloc((1+(N+2)*size)*MAX_INTRASIZE*sizeof(double));
  intra_data = (double*)malloc(N*MAX_INTRASIZE*sizeof(double));

  //recvdata for each process is the final data received by the process
  recvdata = (double*)malloc((N+1)*(size+1)*sizeof(double));

  //sendcounts contain the nuymber of data doubles to be sent to each process in alltoallv
  sendcounts = (int*)malloc((size+1)*sizeof(int));
  sdispls = (int*)malloc((size+1)*sizeof(int));

  //Random initialization of sendcounts
  for(int i=0;i<size;i++){
    sendcounts[i] = rand()%N;
  }
  sdispls[0] = 0;
  for(int i=1;i<size;i++){
    sdispls[i] = sdispls[i-1] + sendcounts[i-1];
  }
  recvcounts = (int*)malloc(size*sizeof(int));
  rdispls = (int*)malloc(size*sizeof(int));

  //Random initialization of data
  for(int i=0;i<N;i++)
      data[i] = rand()%N;


  FILE* fptr;
  //Opening data file for outputting execution time
  if(!myrank)
    fptr = fopen("data","a");

  //Default MPI_Bcast
    sTime = MPI_Wtime();
    default_bcast(5);
    eTime = MPI_Wtime();
    default_bcast_time = (eTime - sTime)/5;

    //Optimized Bcast
    sTime = MPI_Wtime();
    optimized_bcast(5);
    eTime = MPI_Wtime();
    optimized_bcast_time = (eTime - sTime)/5;

    //Default MPI_Reduce
    sTime = MPI_Wtime();
    default_reduce(5);
    eTime = MPI_Wtime();
    default_reduce_time = (eTime - sTime)/5;

    //Optimized MPI_Reduce
    sTime = MPI_Wtime();
    optimized_reduce(5);
    eTime = MPI_Wtime();
    optimized_reduce_time = (eTime - sTime)/5;

    //Default MPI_Gather
    sTime = MPI_Wtime();
    default_gather(5);
    eTime = MPI_Wtime();
    default_gather_time = (eTime - sTime)/5;

    //Optimized MPI_Gather
    sTime = MPI_Wtime();
    optimized_gather(5);
    eTime = MPI_Wtime();
    optimized_gather_time = (eTime - sTime)/5;

    //Default MPI_Alltoallv
    sTime = MPI_Wtime();
    default_alltoallv(5);
    eTime = MPI_Wtime();
    default_alltoallv_time = (eTime - sTime)/5;

//    //Optimized MPI_Alltoallv1
//    sTime = MPI_Wtime();
//    optimized_alltoallv_v1(5);
//    eTime = MPI_Wtime();
//    optimized_alltoallv_time = (eTime - sTime)/5;
//  
    //Optimized MPI_Alltoallv2
    sTime = MPI_Wtime();
    optimized_alltoallv_v2(5);
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

