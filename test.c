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
    if(disk_close()!=1){
        printf("Error2\n");
        return -1;
    }
}