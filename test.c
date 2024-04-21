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
    //f_test(1);
    printf("Test f_open\n");
    File* file = f_open("file.txt","r");
    if(file==NULL){printf("Can't open file\n");}
    else{printf("File open successfully\n");}

   char content[38];
    f_read(file,content,38);
    printf("%s\n",content);
    f_close(file);

    File* test = f_open("test.txt","r");
    if(test!=NULL){printf("Wierd Behave\n");}

    test = f_open("test.txt","w");
    if(test==NULL){}
    else{
        f_write(test,content,37);
        f_close(test);
        test = f_open("test.txt","r");
        char test_content[38];
        f_read(test,test_content,38);
        printf("%s\n",test_content);
        f_close(test);
    }
    
    test = f_open("test.txt","a+");
    if(test==NULL){}
    else{
        f_write(test,content,38);
        f_rewind(test);
        f_test(3,0);
        char new_content[76];
        f_read(test,new_content,76);
        printf("%c\n",new_content[70]);
        f_close(test);
    }
    

    if(disk_close()!=1){
        printf("Error2\n");
        return -1;
    }
}