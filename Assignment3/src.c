#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0, read_len = 0;
    char* delim = ",";
    char* token;
    float* data;

   fp = fopen("tdata_5.csv", "r");
   if(fp == NULL)
     exit(1);

   int nrows = -1;
   while((read_len = getline(&line, &len, fp)) != -1){
     nrows++;
   }
   fseek(fp,0,SEEK_SET);

   read_len = getline(&line,&len,fp);
   int ncols = -2;
   if(read_len != -1){
    line[read_len-1] = 0;
    token = strtok(line,delim);
    while(token != NULL){
      ncols++;
      token = strtok(NULL,delim);
    }
   }
   printf("%d %d\n",nrows,ncols);

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
   free(line);

//   for(int i=0;i<nrows;i++){
//     for(int j=0;j<ncols;j++){
//       printf("%lf ",data[i*ncols+j]);
//     }
//     printf("\n\n");
//   }




    exit(EXIT_SUCCESS);
}
