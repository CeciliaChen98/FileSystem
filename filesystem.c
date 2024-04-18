#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_CURRENT 0
#define PATH_ROOT 1
#define FILE_TYPE 0
#define DIRECTORY_TYPE 1
#define MAX_INPUT_SIZE 64

struct Tokenizer{
    int flag;
    int length; 
    char* tokens[MAX_INPUT_SIZE];
};

// helper methods:
static struct inode* getInode(int index){
    return (struct inode*) inode_data + sb->size * index; 
}

static void* getData(int index){
    return (void*) inode_data + sb->size * index; 
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
	diskimage = fopen(diskname,"w+r");
    if(diskimage==NULL){return -1;}

    fread(sb,512,1,diskimage);
    fread(inode_data,sb->size*sb->data_offset,1,diskimage);

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
    void* data = getData(temp_inode->dblocks[0]);
    int index = 0;

    while(index<temp_inode->size){
        memcpy(,data,sb->size);
    }

    File* file = (File*)malloc(sizeof(File));
    file->block_index = 0;
    file->position = 0;
    
    strcpy(file->name,filename);

    free(path);
    return file;
    // check if the file exists according to mode

    // if mode is read, and the file doesn’t exist
	// if mode is append/writing, and the file doesn’t exist 
    // create a new inode
    // create a new FILE, set its mode, and add it to the open_list
}

struct dirent* f_opendir(char* directory){
    // do the path standardizing and permission check
    struct Tokenizer* path = tokenize(directory);
    // if the path is invalid (exceeds MAX_INPUT_SIZE)
    if(path==NULL){
        return NULL;
    }
    if(path->flag==PATH_CURRENT){
        

    }else{

    }
	
    // find the dirent of target directory and return it
    // if the directory is not existing
    // if the directory is not readable (by permission)
}