#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(int argc, char* argv[]){
    if(disk_open("diskimage")!=1){
        printf("Error1\n");
        return -1;
    }
    f_test(0);
    printf("Test f_open\n");
    File* file = f_open("file.txt","r");
    if(file==NULL){printf("Can't open file");}
    printf("Test f_close\n");
    printf("%d\n",f_close(file));
    if(disk_close()!=1){
        printf("Error2\n");
        return -1;
    }
}