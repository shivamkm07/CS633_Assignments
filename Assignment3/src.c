#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
    FILE *fp;
    char *line = NULL;
    size_t len = 0, read_len = 0;
    char* delim = ",";
    char* token;
    float* data;
    int dim[2];
    int myrows,mycols,nrows,ncols;
    float* recvdata,*local_min,*global_min,*check_min;
    double stime,etime,time;
    char* filename = argv[1];
  float tot_min;

   int myrank,size;
   MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
   MPI_Comm_size(MPI_COMM_WORLD,&size);

   if(myrank == 0){
     fp = fopen(filename, "r");
     if(fp == NULL)
       exit(1);
  
     nrows = -1;
     while((read_len = getline(&line, &len, fp)) != -1){
       nrows++;
     }
     fseek(fp,0,SEEK_SET);
  
     read_len = getline(&line,&len,fp);
     ncols = -2;
     if(read_len != -1){
      line[read_len-1] = 0;
      token = strtok(line,delim);
      while(token != NULL){
        ncols++;
        token = strtok(NULL,delim);
      }
     }
//     printf("%d %d\n",nrows,ncols);
  
     data = (float*)malloc(nrows*ncols*sizeof(float));
  
     int i = 0, j= 0;
     while((read_len = getline(&line, &len, fp)) != -1) {
      line[read_len-1] = 0;
      token = strtok(line,delim);
      token = strtok(NULL,delim);
      token = strtok(NULL,delim);
      j = 0;
      while(token != NULL){
        float num = strtof(token,NULL);
        data[i*ncols+j] = num;
        token = strtok(NULL,delim);
        j++;
      }
      i++;
     }
     dim[0] = nrows/size;
     dim[1] = ncols;
     free(line);

     check_min = (float*)malloc(ncols*sizeof(float));
     for(int i=0;i<ncols;i++){
       check_min[i] = data[0*ncols+i];
     }
     for(int i=1;i<nrows;i++){
       for(int j=0;j<ncols;j++){
         if(data[i*ncols+j] < check_min[j])
           check_min[j] = data[i*ncols+j];
       }
     }
    fclose(fp);
//     printf("closed file read\n");
   }

   stime = MPI_Wtime();

   int niters = 10;
   for(int iter = 0;iter < niters;iter++){

   MPI_Bcast(dim,2,MPI_INT,0,MPI_COMM_WORLD);
   myrows = dim[0];
   mycols = dim[1];
  // printf("myrank %d myrows %d mycols %d\n",myrank,myrows,mycols);
   recvdata = (float*)malloc(myrows*mycols*sizeof(float));
   MPI_Scatter(data,myrows*mycols,MPI_FLOAT,recvdata,myrows*mycols,MPI_FLOAT,0,MPI_COMM_WORLD);
//   printf("Scatter done myrank %d\n",myrank);

//   for(int i=0;i<myrows;i++){
//     printf("myrank %d ",myrank);
//     for(int j=0;j<mycols;j++){
//       printf("%lf ",recvdata[i*mycols+j]);
//     }
//     printf("\n");
//   }


   local_min = (float*)malloc(mycols*sizeof(float));
   for(int i=0;i<mycols;i++)
     local_min[i] = recvdata[0*mycols+i];

//   for(int i=0;i<mycols;i++)
//     printf("%lf ",local_min[i]);
//   printf("myrank %d\n",myrank);
//   printf("locl_mina half myrank %d\n",myrank);
   for(int i=1;i<myrows;i++){
     for(int j=0;j<mycols;j++){
       if(recvdata[i*mycols+j] < local_min[j])
        local_min[j] = recvdata[i*mycols+j];
     }
   }

//   for(int i=0;i<mycols;i++)
//     printf("%lf ",local_min[i]);
//   printf("myrank %d\n",myrank);

   global_min = (float*)malloc(mycols*sizeof(float));

   MPI_Reduce(local_min,global_min,mycols,MPI_FLOAT,MPI_MIN,0,MPI_COMM_WORLD);

   if(myrank == 0){
     for(int i=(nrows/size)*size;i<nrows;i++){
       for(int j=0;j<ncols;j++){
         if(data[i*ncols+j] < global_min[j])
           global_min[j] = data[i*ncols+j];
       }
     }
     tot_min = global_min[0];
     for(int i=0;i<mycols;i++){
       if(global_min[i] < tot_min)
         tot_min = global_min[i];
     }
   }
   }
   etime = MPI_Wtime();
   time = (etime - stime)/niters;
   double maxtime;
   MPI_Reduce(&time,&maxtime,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
   FILE* fptr;
   if(myrank == 0){
     fptr = fopen("output.txt","w");
     for(int i=0;i<mycols;i++){
       fprintf(fptr,"%.2lf",global_min[i]);
       if(i < mycols-1)
         fprintf(fptr,",");
       else
         fprintf(fptr,"\n");
//       printf("%.2lf ",global_min[i]);
     }
     for(int i=0;i<mycols;i++)
       if(check_min[i] != global_min[i])
         printf("error\n");
    fprintf(fptr,"%.2lf\n",tot_min);
    fprintf(fptr,"%.2lf\n",maxtime);
//     printf("%.2lf\n",tot_min);
//     printf("%.2lf\n",maxtime);
    fclose(fptr);
   }







   MPI_Finalize();


    exit(EXIT_SUCCESS);
}
