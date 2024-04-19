#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define PATH_CURRENT 0
#define PATH_ROOT 1
#define FILE_TYPE 0
#define DIRECTORY_TYPE 1
#define MAX_INPUT_SIZE 64

enum Mode {
    None = -1,
    READ,
    WRITE,
    APPEND,
    READ_WRITE, //file must exist
    WRITE_READ,
    APPEND_READ
};

struct Tokenizer{
    int flag;
    int length; 
    char* tokens[MAX_INPUT_SIZE];
};

// helper methods:
static struct inode* getInode(int index){
    return inode_data + index;
}

static char* getData(int index){
    return (char*) inode_data + block_size * index; 
}

static struct dirent* findDirentByIndex(struct inode inode, int INDEX) {
    int read_num = 0;
    int count = 0;  // This will count the number of directory entries processed

    // Loop through direct blocks
    for (int i = 0; i < N_DBLOCKS; i++) {
        int block_num = 0;
        char* data = getData(inode.dblocks[i]);
        while (block_num < block_size) {
            struct dirent* cur_dirent = (struct dirent*)data;
            
            if (count == INDEX + 2) {  // Check if the current count matches the desired INDEX
                return cur_dirent;
            }
            
            read_num += sizeof(struct dirent);
            block_num += sizeof(struct dirent);
            if (read_num >= inode.size) {
                return NULL;  // No more entries to read
            }
            
            data = (char*)data + sizeof(struct dirent);
            count++;  // Increment the directory entry counter
        }
    }

    // Loop through indirect blocks
    for (int i = 0; i < N_IBLOCKS; i++) {
        int index = 0;
        int* index_data = (int*)getData(inode.iblocks[i]);
        while (index < block_size / sizeof(int)) {
            int block_num = 0;
            char* data = getData(index_data[index]);
            while (block_num < block_size) {
                struct dirent* cur_dirent = (struct dirent*)data;

                if (count == INDEX + 2) {  // Check if the current count matches the desired INDEX
                    return cur_dirent;
                }

                read_num += sizeof(struct dirent);
                block_num += sizeof(struct dirent);
                if (read_num >= inode.size) {
                    return NULL;  // No more entries to read
                }
                
                data = (char*)data + sizeof(struct dirent);
                count++;  // Increment the directory entry counter
            }
            index++;
        }
    }

    return NULL;  // If no dirent was found at the specified index
}

static void* appendPosition(struct inode* inode){
    // calculate the offset
    int block_num = inode->size / block_size;
    int last_block = block_num - 1;
    int offset = inode->size % block_size;

    // if need to assign a new data block
    if(offset == 0){
        last_block++;
        // rearrange the free data block 
        // if no more free data block
        if(sb->free_block==-1){return NULL;}
        int head_free = *(int*)getData(sb->free_block);
        int data_file;
        if(head_free==-1){
            data_file = sb->free_block;
            sb->free_block = -1;
        }else{
            data_file = head_free;
            int next_free = *(int*)getData(head_free);
            int* ptr = (int*)getData(sb->free_block);
            *ptr = next_free;
        }
        if(last_block<N_DBLOCKS){
            inode->dblocks[last_block] = data_file;
            return getData(data_file);
        }
        last_block = last_block-N_DBLOCKS;
        for(int i=0;i<N_IBLOCKS;i++){
            if(last_block<block_size/sizeof(int)){
                int* ptr = (int*)getData(inode->iblocks[i])+last_block;
                *ptr = data_file;
                return getData(data_file);
            }else{
                last_block = last_block - block_size/sizeof(int);
            }
        }
    }else{
        if(last_block<N_DBLOCKS){
            return getData(inode->dblocks[last_block])+offset;
        }
        last_block = last_block-N_DBLOCKS;
        for(int i=0;i<N_IBLOCKS;i++){
            if(last_block<block_size/sizeof(int)){
                return getData((int*)getData(inode->iblocks[i])+last_block)+offset;
            }else{
                last_block = last_block - block_size/sizeof(int);
            }
        }
    }
}

static struct dirent* createFile(struct dirent* cur_dirent){

    // we don't assign any data block for a new file

    // rearrange the free inode 
    int free_i = getInode(sb->free_inode)->parent;
    struct inode* file_inode;
    if(free_i!=-1){
        int next_i = getInode(free_i)->parent;
        file_inode = getInode(free_i);
        getInode(sb->free_inode)->parent = next_i;
    }else{
        file_inode = getInode(sb->free_inode);
        sb->free_inode = -1;
    }
    // modify the inode to store all the information
    file_inode -> type = FILE_TYPE;
    file_inode -> permissions = READ_WRITE;
    file_inode -> parent = cur_dirent->inode;
    file_inode -> nlink = 1;
    file_inode -> size = 0;
    file_inode -> mtime = time(NULL);    

    struct dirent direct*  

}

static struct dirent* findDirent(struct inode inode, char* target, int type){
    int read_num = 0;
    // loop through direct blocks
    for(int i = 0;i<N_DBLOCKS;i++){
        int block_num = 0;
        char* data = getData(inode.dblocks[i]);
        while(block_num<sb->size){
            struct dirent* cur_dirent = (struct dirent*)data;
            // if find the desired one
            if (strcmp(target,cur_dirent->name)==0&&cur_dirent->type==type){
                return cur_dirent;
            }
            read_num += sizeof(struct dirent);
            block_num += sizeof(struct dirent);
            if(read_num>= inode.size){
                return NULL;
            }
            data = (char*)data + sizeof(struct dirent*);
        }
    }
    // loop through indirect blocks
    for(int i = 0;i<N_IBLOCKS;i++){
        int index = 0;
        int* index_data = (int*)getData(inode.iblocks[i]);
        while(index < sb->size/sizeof(int)){
            int block_num = 0;
            char* data = getData(index_data[index]);
            while(block_num<sb->size){
                struct dirent* cur_dirent = (struct dirent*)data;
                // if find the desired one
                if (strcmp(target,cur_dirent->name)==0&&cur_dirent->type==type){
                    return cur_dirent;
                }
                read_num += sizeof(struct dirent);
                block_num += sizeof(struct dirent);
                if(read_num>= inode.size){
                    return NULL;
                }
                data = (char*)data + sizeof(struct dirent*);
            }
            index++;
        }
    }
    return NULL;
}

static struct Tokenizer* tokenize(char* arg){
    // initialize a toeknizer
    struct Tokenizer* path = (struct Tokenizer*)malloc(sizeof(struct Tokenizer));
    int i = 0;
    if(arg[i]=='/'||arg[i]=='~'){
        path->flag = PATH_ROOT;
        i++;
    }else{
        path->flag = PATH_CURRENT;
    }
    char* arg_copy = strdup(arg+i);
    char* token = strtok(arg_copy,"/");

    int arg_count = 0;
    // tokenize input
    while (token != NULL) {
        if(arg_count>=MAX_INPUT_SIZE){
            free(path);
            return NULL;
        }
        path->tokens[arg_count] = token;
        arg_count++;
        token = strtok(NULL, "/");
    } 
    free(arg_copy); 
    path->length = arg_count;
    return path;
}

int disk_open(char *diskname){
    // open the disk image
	diskimage = fopen(diskname,"r+b");
    if(diskimage==NULL){return -1;}

    fread(sb,512,1,diskimage);
    block_size = sb->size;
    fread(inode_data,block_size*sb->data_offset,1,diskimage);
    current_direct->inode = 0;
    current_direct->type = DIRECTORY_TYPE;
    strcpy(current_direct->name,"root");
}

int disk_close(){

    fclose(diskimage);
} 

File* f_open(char* filename, char* mode){
	// do the path standardizing and permission check
    struct Tokenizer* path = tokenize(filename);
    // if the path is invalid (exceeds MAX_INPUT_SIZE)
    if(path==NULL){
        return NULL;
    }
    // check if there is a directory needed to open
    struct dirent* target = current_direct;
    if(path->length>1){
        char * directname;
        for(int i=0;i<path->length-1;i++){
            strcat(directname,path->tokens[i]);
        }
        target = f_opendir(directname);
    }
    struct inode* temp_inode = getInode(target->inode);
    target = findDirent(*temp_inode,path->tokens[path->length-1],FILE_TYPE);
    int permission = NONE;
    if(target!=NULL){permission = getInode(target->inode)->permissions;}
    free(path);

    // mode
    int int_mode = -1;
    if(strcmp("r",mode)==0&&(permission==READ_ONLY||permission==READ_WRITE)){int_mode = READ;}
    else if(strcmp("r+",mode)==0&&permission==READ_WRITE){int_mode = READ_WRITE;}
    else if(strcmp("w",mode)==0){int_mode = WRITE;}
    else if(strcmp("w+",mode)==0){int_mode = WRITE_READ;}
    else if(strcmp("a",mode)==0){int_mode = APPEND;}
    else if(strcmp("a+",mode)==0&&permission==READ_WRITE){int_mode = APPEND_READ;}
    else{
        printf("Invalid mode\n");
        return NULL;
    }
    File* file = (File*)malloc(sizeof(File));
    file->block_index = 0;
    file->position = 0;
    file->inode = target->inode;

    if(int_mode==READ || int_mode==READ_WRITE){
        if(target==NULL){
            printf("File does not exist");
            free(file);
            return NULL;
        }
    }
    else if(int_mode==WRITE || int_mode== WRITE_READ){
        if(target==NULL){

        }else{
            
        }
    }

    file->mode = int_mode;
    strcpy(file->name,filename);

    return file;
    // check if the file exists according to mode

    // if mode is read, and the file doesn’t exist
	// if mode is append/writing, and the file doesn’t exist 
    // create a new inode
    // create a new FILE, set its mode, and add it to the open_list
}

// return null if not found, return pointer to the dirent if found
struct dirent* f_opendir(char* directory) {
    // Tokenize the input path
    struct Tokenizer* path = tokenize(directory);
    if (path == NULL) {
        return NULL;
    }

    struct dirent* cur = NULL;

    // Decide starting directory based on path flag
    if (path->flag == PATH_CURRENT) {
        cur = current_direct;
    } else {
        // Assuming the only other option is to start from root
        cur = root_direct;
    }

    // Iterate over each token based on the number of tokens
    for (int count = 0; count < path->length; count++) {
        if (cur == NULL) {
            printf("Directory not found\n");
            return NULL;
        }
        // Perform directory matching or traversal
        if (strcmp(cur->name, path->tokens[count]) == 0) {
            // If the directory name matches the current token, continue to next token
            continue;
        }
        // Move to the next directory in the path
        cur = findDirent(inode_data[cur->inode], path->tokens[count], DIRECTORY_TYPE);
    }

    cur->offset = 0;

    // Return the final directory entry found, or NULL if not found
    return cur;
}

struct dirent* f_readdir(struct dirent* directory){
	// read the current sub-directory according to the offset
	// update the offset;
    return findDirentByIndex(*getInode(directory->inode), directory->offset);
    directory->offset ++;
}

int f_closedir(struct dirent* directory) {
	// if directory doesn’t exists
    if (directory == NULL) {
        return -1;
    }
	// set the offset of the directory to 0
    directory->offset = 0;
	return 0;
}

int f_rmdir(char* path_name) {
	// if path_name is not existing
    // Tokenize the input path
    struct Tokenizer* path = tokenize(path_name);
    if (path == NULL) {
        return NULL;
    }

    struct dirent* cur = NULL;

    // Decide starting directory based on path flag
    if (path->flag == PATH_CURRENT) {
        cur = current_direct;
    } else {
        // Assuming the only other option is to start from root
        cur = root_direct;
    }

    // Iterate over each token based on the number of tokens
    for (int count = 0; count < path->length; count++) {
        if (cur == NULL) {
            printf("Directory not found\n");
            return NULL;
        }
        // Perform directory matching or traversal
        if (strcmp(cur->name, path->tokens[count]) == 0) {
            // If the directory name matches the current token, continue to next token
            continue;
        }
        // Move to the next directory in the path
        cur = findDirent(inode_data[cur->inode], path->tokens[count], DIRECTORY_TYPE);
    }

    // find its parent directory and find the desired dirent


    // delete all the files and directories inside the dirent, and remove the dirent from the direct_list
    //delete the dirent from its parent’s data block
    return 1;
}



