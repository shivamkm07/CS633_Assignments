// Timing codes

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

double* data,*data1;
double *top,*bottom,*left,*right;
int N,row,col,psize,myrank,size;
MPI_Request* request;
MPI_Status* status;

void multiple_mpi_send(){   //Each MPI_Send transmits only 1 element(1 double)
  int index = 0;
  if(row > 0){    // If the current process has another process located above
  for(int i=0;i<N;i++){
    MPI_Isend(&data[0*N+i], 1, MPI_DOUBLE, (row-1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    //(row-1)*psize + col is the rank of the process located above
    index++;
    MPI_Irecv(&top[i], 1, MPI_DOUBLE, (row-1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }
  if(row < psize-1){    // If the current process has another process located below
  for(int i=0;i<N;i++){
    MPI_Isend(&data[(N-1)*N+i], 1, MPI_DOUBLE, (row+1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    //(row+1)*psize+col is the rank of the process located below
    index++;
    MPI_Irecv(&bottom[i], 1, MPI_DOUBLE, (row+1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }

  if(col > 0){    // If the current process has another process located at left
  for(int i=0;i<N;i++){
    MPI_Isend(&data[i*N+0], 1, MPI_DOUBLE, myrank - 1, i, MPI_COMM_WORLD, &request[index]);
    //myrank -1 is the rank of the process located left
    index++;
    MPI_Irecv(&left[i], 1, MPI_DOUBLE, myrank - 1, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }

  if(col < psize-1){    // If the current process has another process located in right
  for(int i=0;i<N;i++){
    MPI_Isend(&data[i*N+(N-1)], 1, MPI_DOUBLE, myrank + 1, i, MPI_COMM_WORLD, &request[index]);
    //myrank + 1 is the rank of process located right
    index++;
    MPI_Irecv(&right[i], 1, MPI_DOUBLE, myrank + 1, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }

  //Waiting for all Isends and IRecvs to complete
  MPI_Waitall(index,request,status);

}

void mpi_pack_send(){   //Using MPI_Pack/MPI_Unpack and MPI_Isend/MPI_IRecv to transmit multiple elements at a time
  int index = 0;
  double* top_sendbuffer,*top_recvbuffer,*bot_sendbuffer,*bot_recvbuffer;
  double *left_sendbuffer,*left_recvbuffer,*right_sendbuffer,*right_recvbuffer;
  if(row > 0){    // If the current process has another process located above
    top_sendbuffer = (double*)malloc(N*sizeof(double));
    top_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    //Packing the topmost row into top_senduffer
    MPI_Pack(&data[0*N+0],N,MPI_DOUBLE,top_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(top_sendbuffer,pos,MPI_PACKED,(row-1)*psize+col,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(top_recvbuffer,N*sizeof(double),MPI_PACKED,(row-1)*psize+col,(row-1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(row < psize-1){    // If the current process has another process located below
    bot_sendbuffer = (double*)malloc(N*sizeof(double));
    bot_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    //Packing the bottommost column into bot_sendbuffer
    MPI_Pack(&data[(N-1)*N+0],N,MPI_DOUBLE,bot_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(bot_sendbuffer,pos,MPI_PACKED,(row+1)*psize+col,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(bot_recvbuffer,N*sizeof(double),MPI_PACKED,(row+1)*psize+col,(row+1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col > 0){    // If the current process has another process located left
    left_sendbuffer = (double*)malloc(N*sizeof(double));
    left_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    //Packing the leftmost column into left_sendbuffer
    for(int i=0;i<N;i++)
      MPI_Pack(&data[i*N+0],1,MPI_DOUBLE,left_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(left_sendbuffer,pos,MPI_PACKED,myrank-1,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(left_recvbuffer,N*sizeof(double),MPI_PACKED,myrank-1,myrank-1,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col < psize-1){    // If the current process has another process located right
    right_sendbuffer = (double*)malloc(N*sizeof(double));
    right_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    //Packing the rightmost column into right_sendbuffer
    for(int i=0;i<N;i++)
      MPI_Pack(&data[i*N+(N-1)],1,MPI_DOUBLE,right_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(right_sendbuffer,pos,MPI_PACKED,myrank+1,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(right_recvbuffer,N*sizeof(double),MPI_PACKED,myrank+1,myrank+1,MPI_COMM_WORLD,&request[index]); index++;
  }
  MPI_Waitall(index,request,status);

  if(row > 0){
    int pos = 0;
    //Unpacking the row above topmost row
    MPI_Unpack(top_recvbuffer,N*sizeof(double),&pos,top,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
  if(row < psize-1){
    int pos = 0;
    //Unpacking the row below bottommost row
    MPI_Unpack(bot_recvbuffer,N*sizeof(double),&pos,bottom,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
  if(col > 0){
    int pos = 0;
    //Unpacking the column left to the leftmost column
    MPI_Unpack(left_recvbuffer,N*sizeof(double),&pos,left,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
  if(col < psize-1){
    int pos = 0;
    //Unpacking the column right to the rightmost column
    MPI_Unpack(right_recvbuffer,N*sizeof(double),&pos,right,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
}

void mpi_derived_send(){    //Using Derived data types for sending multiple elements in one go

  MPI_Datatype rowtype,coltype;

  //rowtype represents one row of the matrix i.e. N contiguous elements
  MPI_Type_contiguous(N,MPI_DOUBLE,&rowtype);
  //coltype represents one column of matrix  i.e. N elements with stride of N between every two consecutive elements
  MPI_Type_vector(N,1,N,MPI_DOUBLE,&coltype);
  MPI_Type_commit(&rowtype);
  MPI_Type_commit(&coltype);
  int index = 0;

  if(row > 0){    // If the current process has another process located above
    MPI_Isend(&data[0*N+0], 1, rowtype, (row-1)*psize+col, myrank, MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(top,N,MPI_DOUBLE,(row-1)*psize+col,(row-1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(row < psize-1){    // If the current process has another process located below
    MPI_Isend(&data[(N-1)*N+0], 1, rowtype, (row+1)*psize+col, myrank, MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(bottom,N,MPI_DOUBLE,(row+1)*psize+col,(row+1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col > 0){    // If the current process has another process located left
    MPI_Isend(&data[0*N+0], 1, coltype, myrank-1 , myrank , MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(left,N,MPI_DOUBLE,myrank-1, myrank-1 ,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col < psize-1){    // If the current process has another process located right
    MPI_Isend(&data[0*N+(N-1)], 1, coltype, myrank+1 , myrank , MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(right,N,MPI_DOUBLE,myrank+1, myrank+1 ,MPI_COMM_WORLD,&request[index]); index++;
  }
  MPI_Waitall(index,request,status);
}



void stencil_compute(){
  //Computing averages for all the non-boundary elements
    for(int i=1;i<(N-1);i++)
      for(int j=1;j<(N-1);j++)
        data1[i*N+j] = (data[i*N+j+1] + data[i*N+j-1] + data[(i+1)*N+j] + data[(i-1)*N+j])/4;
    
      for(int i=1;i<(N-1);i++){
  
        //If the current processs has top neighbor, use the 'top' array, otherwise not
        if(row > 0)
          data1[0*N+i] = (data[0*N+(i-1)] + data[0*N+(i+1)] + data[1*N+i] + top[i])/4;
        else
          data1[0*N+i] = (data[0*N+(i-1)] + data[0*N+(i+1)] + data[1*N+i])/3;
  
        //If the current processs has bottom neighbor, use the 'bottom' array, otherwise not
        if(row < psize-1)
          data1[(N-1)*N+i] = (data[(N-1)*N+(i-1)] + data[(N-1)*N+(i+1)] + data[(N-2)*N+i] + bottom[i])/4;
        else
          data1[(N-1)*N+i] = (data[(N-1)*N+(i-1)] + data[(N-1)*N+(i+1)] + data[(N-2)*N+i])/3;
  
        //If the current processs has left neighbor, use the 'left' array, otherwise not
        if(col > 0)
          data1[i*N+0] = (data[(i-1)*N+0] + data[(i+1)*N+0] + data[i*N+1] + left[i])/4;
        else
          data1[i*N+0] = (data[(i-1)*N+0] + data[(i+1)*N+0] + data[i*N+1] )/3;
  
        //If the current processs has right neighbor, use the 'right' array, otherwise not
        if(col < psize-1)
          data1[i*N+(N-1)] = (data[(i+1)*N+(N-1)] + data[(i-1)*N+(N-1)] + data[i*N+(N-2)] + right[i])/4;
        else
          data1[i*N+(N-1)] = (data[(i+1)*N+(N-1)] + data[(i-1)*N+(N-1)] + data[i*N+(N-2)] )/3;
  
    }

      //Computation for element data[0][0]
      if(row > 0){
        if(col > 0)
          data1[0*N+0] = (data[1*N+0] + data[0*N+1] + top[0] + left[0])/4;
        else
          data1[0*N+0] = (data[1*N+0] + data[0*N+1] + top[0])/3;
      }else{
        if(col > 0)
          data1[0*N+0] = (data[1*N+0] + data[0*N+1] + left[0])/3;
        else
          data1[0*N+0] = (data[1*N+0] + data[0*N+1])/2;
      }
  
      //Computation for element data[0][N-1]
      if(row > 0){
        if(col < psize-1)
          data1[0*N+(N-1)] = (data[0*N+(N-2)] + data[1*N+(N-1)] + top[(N-1)] + right[0])/4;
        else
          data1[0*N+(N-1)] = (data[0*N+(N-2)] + data[1*N+(N-1)] + top[(N-1)])/3;
      }else{
        if(col < psize-1)
          data1[0*N+(N-1)] = (data[0*N+(N-2)] + data[1*N+(N-1)] + right[0])/3;
        else
          data1[0*N+(N-1)] = (data[0*N+(N-2)] + data[1*N+(N-1)])/2;
      }
  
      //Computation for element data[N-1][0]
      if(row < psize-1){
        if(col > 0)
          data1[(N-1)*N+0] = (data[(N-1)*N+1] + data[(N-2)*N+0] + bottom[0] + left[(N-1)])/4;
        else
          data1[(N-1)*N+0] = (data[(N-1)*N+1] + data[(N-2)*N+0] + bottom[0])/3;
      }else{
        if(col > 0)
          data1[(N-1)*N+0] = (data[(N-1)*N+1] + data[(N-2)*N+0] + left[(N-1)])/3;
        else
          data1[(N-1)*N+0] = (data[(N-1)*N+1] + data[(N-2)*N+0] )/2;
        }
  
      //Computation for element data[N-1][N-1]
      if(row < psize-1){
        if(col < psize-1)
          data1[(N-1)*N+(N-1)] = (data[(N-1)*N+(N-2)] + data[(N-2)*N+(N-1)] + bottom[(N-1)] + right[(N-1)])/4;
        else
          data1[(N-1)*N+(N-1)] = (data[(N-1)*N+(N-2)] + data[(N-2)*N+(N-1)] + bottom[(N-1)] )/3;
      }else{
        if(col < psize-1)
          data1[(N-1)*N+(N-1)] = (data[(N-1)*N+(N-2)] + data[(N-2)*N+(N-1)]  + right[(N-1)])/3;
        else
          data1[(N-1)*N+(N-1)] = (data[(N-1)*N+(N-2)] + data[(N-2)*N+(N-1)] )/2;
      }
  

      //Swapping the data1 and data array, so that data points to the computed values
      double* temp = data;
      data = data1;
      data1 = temp;
}

int main( int argc, char *argv[])
{
  double sTime, eTime, time, maxTime;

  MPI_Init(&argc, &argv);

  int num_data_points = atoi(argv[1]);
  int num_time_steps = atoi(argv[2]);

  //N is the number of rows in the data matrix i.e. N*N = num_data_points
  N = (int)sqrt(num_data_points);
  
  //'data' for each process is the subdomain data that every process contains
  data = (double*)malloc(N*N*sizeof(double));
  //'data1' temporary holds the values calculated in stencil computation
  data1 = (double*)malloc(N*N*sizeof(double));


  //Random initialization of data
  for(int i=0;i<N;i++)
    for(int j=0;j<N;j++){
      data[i*N+j] = rand()%num_data_points;
    }

  //top stores the data received from top neighbor
  top = (double*)malloc(N*sizeof(double));
  //bottom stores the data received from bottom neighbor
  bottom = (double*)malloc(N*sizeof(double));
  //left stores the data received from left neighbor
  left = (double*)malloc(N*sizeof(double));
  //right stores the data received from right neighbor
  right = (double*)malloc(N*sizeof(double)); 

  request = (MPI_Request*)malloc(8*N*sizeof(MPI_Request));
  status = (MPI_Status*)malloc(8*N*sizeof(MPI_Status));

  MPI_Comm_rank(MPI_COMM_WORLD, &myrank) ;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  //psize is the number of rows in the 2D matrix of processes
  psize = (int)sqrt((double)size);

  //row,col marks the position of current process in the 2D matrix of processes
  row = myrank/psize;
  col = myrank%psize;

  FILE* fptr;
  //Opening data file for outputting execution time
  if(!myrank)
    fptr = fopen("data","a");

  time = 0;


  for(int method = 0; method < 3; method++){

    time = 0;

    for(int t = 0;t < num_time_steps;t++){
    sTime = MPI_Wtime();

    if(method == 0)
      multiple_mpi_send();
    else if(method == 1)
      mpi_pack_send();
    else 
      mpi_derived_send();

    stencil_compute();
    eTime = MPI_Wtime();
    //Summing computation + communication time for all the time steps per method
    time += eTime - sTime;
    }
  
    // obtain max time
    MPI_Reduce (&time, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (!myrank) 
    {
      //Printing time in data file
    fprintf (fptr,"%lf\n", maxTime);
    //Printing time on stdout in more readable form
    printf("Method %d P %d N %d time %lf\n",method, size, N,maxTime);
    }
  }

  if(!myrank)
    fclose(fptr);


  MPI_Finalize();
  return 0;

}

