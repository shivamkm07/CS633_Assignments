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

void mpi_derived_dtype(){

  MPI_Datatype rowtype,coltype;
  MPI_Type_contiguous(N,MPI_DOUBLE,&rowtype);
  MPI_Type_vector(N,1,N,MPI_DOUBLE,&coltype);
  MPI_Type_commit(&rowtype);
  MPI_Type_commit(&coltype);
  int index = 0;

  if(row > 0){
    MPI_Isend(&data[0*N+0], 1, rowtype, (row-1)*psize+col, myrank, MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(top,N,MPI_DOUBLE,(row-1)*psize+col,(row-1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(row < psize-1){
    MPI_Isend(&data[(N-1)*N+0], 1, rowtype, (row+1)*psize+col, myrank, MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(bottom,N,MPI_DOUBLE,(row+1)*psize+col,(row+1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col > 0){
    MPI_Isend(&data[0*N+0], 1, coltype, myrank-1 , myrank , MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(left,N,MPI_DOUBLE,myrank-1, myrank-1 ,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col < psize-1){
    MPI_Isend(&data[0*N+(N-1)], 1, coltype, myrank+1 , myrank , MPI_COMM_WORLD, &request[index]); index++;
    MPI_Irecv(right,N,MPI_DOUBLE,myrank+1, myrank+1 ,MPI_COMM_WORLD,&request[index]); index++;
  }
  MPI_Waitall(index,request,status);
}

void mpi_pack_unpack(){
  int index = 0;
  double* top_sendbuffer,*top_recvbuffer,*bot_sendbuffer,*bot_recvbuffer;
  double *left_sendbuffer,*left_recvbuffer,*right_sendbuffer,*right_recvbuffer;
  if(row > 0){
    top_sendbuffer = (double*)malloc(N*sizeof(double));
    top_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    MPI_Pack(&data[0*N+0],N,MPI_DOUBLE,top_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(top_sendbuffer,pos,MPI_PACKED,(row-1)*psize+col,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(top_recvbuffer,N*sizeof(double),MPI_PACKED,(row-1)*psize+col,(row-1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(row < psize-1){
    bot_sendbuffer = (double*)malloc(N*sizeof(double));
    bot_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    MPI_Pack(&data[(N-1)*N+0],N,MPI_DOUBLE,bot_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(bot_sendbuffer,pos,MPI_PACKED,(row+1)*psize+col,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(bot_recvbuffer,N*sizeof(double),MPI_PACKED,(row+1)*psize+col,(row+1)*psize+col,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col > 0){
    left_sendbuffer = (double*)malloc(N*sizeof(double));
    left_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    for(int i=0;i<N;i++)
      MPI_Pack(&data[i*N+0],1,MPI_DOUBLE,left_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(left_sendbuffer,pos,MPI_PACKED,myrank-1,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(left_recvbuffer,N*sizeof(double),MPI_PACKED,myrank-1,myrank-1,MPI_COMM_WORLD,&request[index]); index++;
  }
  if(col < psize-1){
    right_sendbuffer = (double*)malloc(N*sizeof(double));
    right_recvbuffer = (double*)malloc(N*sizeof(double));
    int pos = 0;
    for(int i=0;i<N;i++)
      MPI_Pack(&data[i*N+(N-1)],1,MPI_DOUBLE,right_sendbuffer,N*sizeof(double),&pos,MPI_COMM_WORLD);
    MPI_Isend(right_sendbuffer,pos,MPI_PACKED,myrank+1,myrank,MPI_COMM_WORLD,&request[index]); index++;
    MPI_Irecv(right_recvbuffer,N*sizeof(double),MPI_PACKED,myrank+1,myrank+1,MPI_COMM_WORLD,&request[index]); index++;
  }
  MPI_Waitall(index,request,status);

  if(row > 0){
    int pos = 0;
    MPI_Unpack(top_recvbuffer,N*sizeof(double),&pos,top,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
  if(row < psize-1){
    int pos = 0;
    MPI_Unpack(bot_recvbuffer,N*sizeof(double),&pos,bottom,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
  if(col > 0){
    int pos = 0;
    MPI_Unpack(left_recvbuffer,N*sizeof(double),&pos,left,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
  if(col < psize-1){
    int pos = 0;
    MPI_Unpack(right_recvbuffer,N*sizeof(double),&pos,right,N,MPI_DOUBLE,MPI_COMM_WORLD);
  }
}

void multiple_mpisend(){
  int index = 0;
  if(row > 0){
  for(int i=0;i<N;i++){
    MPI_Isend(&data[0*N+i], 1, MPI_DOUBLE, (row-1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    index++;
    MPI_Irecv(&top[i], 1, MPI_DOUBLE, (row-1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }
  if(row < psize-1){
  for(int i=0;i<N;i++){
    MPI_Isend(&data[(N-1)*N+i], 1, MPI_DOUBLE, (row+1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    index++;
    MPI_Irecv(&bottom[i], 1, MPI_DOUBLE, (row+1)*psize+col, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }

  if(col > 0){
  for(int i=0;i<N;i++){
    MPI_Isend(&data[i*N+0], 1, MPI_DOUBLE, myrank - 1, i, MPI_COMM_WORLD, &request[index]);
    index++;
    MPI_Irecv(&left[i], 1, MPI_DOUBLE, myrank - 1, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }

  if(col < psize-1){
  for(int i=0;i<N;i++){
    MPI_Isend(&data[i*N+(N-1)], 1, MPI_DOUBLE, myrank + 1, i, MPI_COMM_WORLD, &request[index]);
    index++;
    MPI_Irecv(&right[i], 1, MPI_DOUBLE, myrank + 1, i, MPI_COMM_WORLD, &request[index]);
    index++;
  }
  }

  MPI_Waitall(index,request,status);

}

void stencil_compute(){
    for(int i=1;i<(N-1);i++)
      for(int j=1;j<(N-1);j++)
        data1[i*N+j] = (data[i*N+j+1] + data[i*N+j-1] + data[(i+1)*N+j] + data[(i-1)*N+j])/4;
    
      for(int i=1;i<(N-1);i++){
  
        if(row > 0)
          data1[0*N+i] = (data[0*N+(i-1)] + data[0*N+(i+1)] + data[1*N+i] + top[i])/4;
        else
          data1[0*N+i] = (data[0*N+(i-1)] + data[0*N+(i+1)] + data[1*N+i])/3;
  
        if(row < psize-1)
          data1[(N-1)*N+i] = (data[(N-1)*N+(i-1)] + data[(N-1)*N+(i+1)] + data[(N-2)*N+i] + bottom[i])/4;
        else
          data1[(N-1)*N+i] = (data[(N-1)*N+(i-1)] + data[(N-1)*N+(i+1)] + data[(N-2)*N+i])/3;
  
        if(col > 0)
          data1[i*N+0] = (data[(i-1)*N+0] + data[(i+1)*N+0] + data[i*N+1] + left[i])/4;
        else
          data1[i*N+0] = (data[(i-1)*N+0] + data[(i+1)*N+0] + data[i*N+1] )/3;
  
        if(col < psize-1)
          data1[i*N+(N-1)] = (data[(i+1)*N+(N-1)] + data[(i-1)*N+(N-1)] + data[i*N+(N-2)] + right[i])/4;
        else
          data1[i*N+(N-1)] = (data[(i+1)*N+(N-1)] + data[(i-1)*N+(N-1)] + data[i*N+(N-2)] )/3;
  
    }
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
  N = (int)sqrt(num_data_points);
  
  data = (double*)malloc(N*N*sizeof(double));
  data1 = (double*)malloc(N*N*sizeof(double));

  for(int i=0;i<N;i++)
    for(int j=0;j<N;j++){
//      data[i*N+j] = 1;
      data[i*N+j] = rand()%num_data_points;
    }

  top = (double*)malloc(N*sizeof(double));
  bottom = (double*)malloc(N*sizeof(double));
  left = (double*)malloc(N*sizeof(double));
  right = (double*)malloc(N*sizeof(double)); 
  request = (MPI_Request*)malloc(8*N*sizeof(MPI_Request));
  status = (MPI_Status*)malloc(8*N*sizeof(MPI_Status));

  MPI_Comm_rank(MPI_COMM_WORLD, &myrank) ;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  psize = (int)sqrt((double)size);
  row = myrank/psize;
  col = myrank%psize;

  time = 0;


  for(int method = 0; method < 3; method++){

    time = 0;

    for(int t = 0;t < num_time_steps;t++){
    sTime = MPI_Wtime();

    if(method == 0)
      multiple_mpisend();
    else if(method == 1)
      mpi_pack_unpack();
    else 
      mpi_derived_dtype();

    stencil_compute();
    eTime = MPI_Wtime();
    time += eTime - sTime;
    }
  
//  for(int i=0;i<N;i++){
//    for(int j=0;j<N;j++)
//      if(data1[i*N+j] != 1)
//        printf("myrank %d i %d j %d\n",myrank,i,j);
//}
    // obtain max time
    MPI_Reduce (&time, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (!myrank) 
      printf ("%lf\n", maxTime);
  }


  MPI_Finalize();
  return 0;

}

