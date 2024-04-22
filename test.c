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
        f_write(test,content,38);
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
        //f_test(3,0);
        printf("print now:\n");
        char new_content[76];
        f_read(test,new_content,76);
        printf("%s\n",new_content);

        // read the second time to get what we have written in the file newly
        f_read(test,new_content,76);
        printf("%s\n",new_content);
        f_close(test);
    }

    //test for f_opendir
    struct dirent* curdir = f_opendir("./");
    if (curdir == NULL) {
        printf("Error when opening the root directory\n");
    }
    printf("the current directory's inode is %d\n", curdir->inode);
    printf("the root directory's inode is %d\n", curdir->inode);
    f_closedir(curdir);

    printf("Testing mkdir\n");
    f_mkdir("~tests");
    //f_test(0,1,0);

    //test for f_stat, test both file and directory
    if (f_stat("test.txt") == -1) {
        printf("Error when checking status of file test.txt\n");
    }
    
    if (f_stat("./") == -1) {
        printf("Error when checking status of the root directory.\n");
    }

    if(disk_close()!=1){
        printf("Error2\n");
        return -1;
    }
}