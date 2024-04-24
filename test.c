#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define DEFAULT_SIZE_MB 1
#define MB_TO_BYTES(mb) ((mb) * 1024 * 1024)
#define BLOCK_SIZE 512
#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct inode))

//export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.

void print_disk_contents() {
    FILE *file = fopen("diskimage", "rb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    struct Superblock sb;
    if (fread(&sb, sizeof(sb), 1, file) != 1) {
        perror("Failed to read superblock");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    printf("Superblock:\n");
    printf("  Size: %d\n  Inode Offset: %d\n  Data Offset: %d\n  Free Inode: %d\n  Free Block: %d\n",
           sb.size, sb.inode_offset, sb.data_offset, sb.free_inode, sb.free_block);

    struct inode in;
    printf("\nInodes:\n");
    fseek(file, (sb.inode_offset+1) * BLOCK_SIZE , SEEK_SET);
    for (int i = 0; i < 8 * INODES_PER_BLOCK; ++i) {
        if (fread(&in, sizeof(struct inode), 1, file) != 1) {
            break;  // Stop if we fail to read an inode
        }
        printf("Inode %d: Type %d, Parent %d, Permissions %d, Size %d, Mtime %d, dblock: %d %d %d\n", 
               i, in.type, in.parent, in.permissions, in.size, in.mtime, in.dblocks[0], in.dblocks[1], in.dblocks[2]);
    }

    // Assuming directory entries are located right at the data offset
    printf("\nDirectory Entries:\n");
    fseek(file, (sb.data_offset+1) * BLOCK_SIZE, SEEK_SET);
    struct dirent de;
    int count = 0;
    while (fread(&de, sizeof(struct dirent), 1, file) == 1) {
        if (strlen(de.name) > 0) {  // Only print valid entries
            printf("  Index: %d, Entry: %s, Inode: %d, Type: %d\n", count/4, de.name, de.inode, de.type);
        }
        count ++;
    }

    fclose(file);
}

int main(int argc, char* argv[]){
    if(disk_open("diskimage")!=1){
        printf("Error1\n");
        return -1;
    }
    //print_disk_contents();

    printf("Test f_open\n");
    File* file = f_open("file.txt","r");
    if(file==NULL){printf("Can't open file\n");}
    else{printf("File open successfully\n");}

    char content[38];
    f_read(file,content,38);
    printf("%s\n",content);
    f_close(file);

    File* test = f_open("test.txt","r");
    if(test!=NULL){printf("Wierd Behave\n");f_close(test);}

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
    printf("Testing f_opendir");
    struct dirent* curdir = f_opendir("./");
    if (curdir == NULL) {
        printf("Error when opening the root directory\n");
    }
    printf("the current directory's inode is %d\n", curdir->inode);
    printf("the root directory's inode is %d\n", curdir->inode);
    f_closedir(curdir);

    printf("\nTesting mkdir\n");
    f_mkdir("~tests");
    //f_test(0,1,0);
    //f_test(0,0,3);

    printf("\nTesting rmdir\n");
    if (f_rmdir("~tests") == -1) {
        printf("Error when removing tests.\n");
    };

    //test for f_stat, test both file and directory
    printf("\nTesting for f_stat.\n");
    if (f_stat("test.txt") == -1) {
        printf("Error when checking status of file test.txt\n");
    }

    printf("\nTesting f_delete\n");
    if(f_delete("test.txt")==-1){
        printf("Error when removing test.txt.\n");
    }
    
    if (f_stat("./") == -1) {
        printf("Error when checking status of the root directory.\n");
    }

    if(disk_close()!=1){
        printf("Error2\n");
        return -1;
    }
    print_disk_contents();
}