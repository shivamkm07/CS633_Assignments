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
    float tot_min;

    char* filename = argv[1]; //Taking data file name as input

   int myrank,size;
   MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
   MPI_Comm_size(MPI_COMM_WORLD,&size);

   if(myrank == 0){
     fp = fopen(filename, "r");
     if(fp == NULL)
       exit(1);
  
     //First calculating number of lines in file, nrows will be equal to number of stations
     nrows = -1;
     //Intialized with -1 to ignore first row containing header
     while((read_len = getline(&line, &len, fp)) != -1){
       nrows++;
     }
     fseek(fp,0,SEEK_SET);//filestream set to starting to start reading and storing actual data
  
     //Reading first header row and using that to calculate number of years for which data is available
     //ncols will finally be equal to the number of years for which data is given
     read_len = getline(&line,&len,fp);
     ncols = -2; //Intialized with -2 to ignore first 2 columns stating latitude and longitude of station
     if(read_len != -1){
      line[read_len-1] = 0;
      token = strtok(line,delim);
      while(token != NULL){
        ncols++;
        token = strtok(NULL,delim);
      }
     }
//     printf("%d %d\n",nrows,ncols);
  
     //Allocating data array to store complete data
     data = (float*)malloc(nrows*ncols*sizeof(float));
  
     int i = 0, j= 0;
     while((read_len = getline(&line, &len, fp)) != -1) {
      line[read_len-1] = 0;
      //Ignoring first two columsn of every row or line
      token = strtok(line,delim);
      token = strtok(NULL,delim);
      token = strtok(NULL,delim);
      j = 0;
      //Iterating over all temperature data in a line
      while(token != NULL){
        float num = strtof(token,NULL);
        //Storing the temperature of ith station in jth year at postion data[i*ncols+j] i.e. data[i][j]
        data[i*ncols+j] = num;
        token = strtok(NULL,delim);
        j++;
      }
      i++;
     }
     //dim stores the dimension of working data for each individual process, dim[0] number of rows that a process needs to work on, dim[1] stores number of years
     dim[0] = nrows/size;
     dim[1] = ncols;
     free(line);
    fclose(fp);
   } //File Reading Completed



   stime = MPI_Wtime();

   int niters = 10;
   //Repeating the execution niters(=10) time and then average time to to reduce noise
   for(int iter = 0;iter < niters;iter++){

     //Broadcasting number of rows and columns to each individual process
   MPI_Bcast(dim,2,MPI_INT,0,MPI_COMM_WORLD);
   myrows = dim[0];
   mycols = dim[1];

   //recvdata stores the working data of each individual process
   recvdata = (float*)malloc(myrows*mycols*sizeof(float));
   //Using MPI_Scatter to distribute data to processes from root process(0)
   MPI_Scatter(data,myrows*mycols,MPI_FLOAT,recvdata,myrows*mycols,MPI_FLOAT,0,MPI_COMM_WORLD);

   //Calculating local minimum yearwuise at each process in parallel
   local_min = (float*)malloc(mycols*sizeof(float));
   for(int i=0;i<mycols;i++)
     local_min[i] = recvdata[0*mycols+i];
   for(int i=1;i<myrows;i++){
     for(int j=0;j<mycols;j++){
       if(recvdata[i*mycols+j] < local_min[j])
        local_min[j] = recvdata[i*mycols+j];
     }
   }


   global_min = (float*)malloc(mycols*sizeof(float));
   //Using MPI_Reduce to reduce the yearwise local minimum into yearwise global minimum
   MPI_Reduce(local_min,global_min,mycols,MPI_FLOAT,MPI_MIN,0,MPI_COMM_WORLD);

   if(myrank == 0){
     //If nrows%size != 0, there will be some unoperated leftover rows in the bottom. Rot proces(0) iterating over that as well to get final global yearwise minima
     for(int i=(nrows/size)*size;i<nrows;i++){
       for(int j=0;j<ncols;j++){
         if(data[i*ncols+j] < global_min[j])
           global_min[j] = data[i*ncols+j];
       }
     }

     //tot_min stores overall global minimum across all stations and across all years
     tot_min = global_min[0];
     for(int i=0;i<mycols;i++){
       if(global_min[i] < tot_min)
         tot_min = global_min[i];
     }
   }
   }
   etime = MPI_Wtime();
   //Averaging time by niters to get averaged time of one execution
   time = (etime - stime)/niters;
   double maxtime;
   //Using MPI_Reduce to calculate the maximum execution time across all processes
   MPI_Reduce(&time,&maxtime,1,MPI_DOUBLE,MPI_MAX,0,MPI_COMM_WORLD);
   FILE* fptr;
   if(myrank == 0){
     fptr = fopen("output.txt","w");
     //Writing yearwise minima in output file
     for(int i=0;i<mycols;i++){
       fprintf(fptr,"%.2lf",global_min[i]);
       if(i < mycols-1)
         fprintf(fptr,",");
       else
         fprintf(fptr,"\n");
     }
     //Writing overall global minima in 2nd lin
    fprintf(fptr,"%.2lf\n",tot_min);
    //Writing execution time in 3rd time
    fprintf(fptr,"%.2lf\n",maxtime);
    fclose(fptr);
   }

   MPI_Finalize();


    exit(EXIT_SUCCESS);
}
