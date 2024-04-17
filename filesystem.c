#include "filesystem.h"
#include <string.h>
#include <stdlib.h>

#define PATH_CURRENT 0
#define PATH_ROOT 1
#define FILE_TYPE 0
#define DIRECTORY_TYPE 1
#define MAX_INPUT_SIZE 64

struct Tokenizer{
    int flag;
    char* tokens[MAX_INPUT_SIZE];
};

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
        path->tokens[arg_count++] = token;
        if(arg_count>MAX_INPUT_SIZE){
            free(path);
            return NULL;
        }
        token = strtok(NULL, "/");
    } 

    free(arg_copy); 
    return path;
}

void disk_initialize(char* diskname){
	
}

File* f_open(char* filename, int mode){
	// do the path standardizing and permission check
    struct Tokenizer* path = tokenize(filename);
    // if the path is invalid (exceeds MAX_INPUT_SIZE)
    if(path==NULL){
        return NULL;
    }
    if(path->flag==PATH_CURRENT){
        

    }else{

    }

    File* file = (File*)malloc(sizeof(File));
    return file;
    // check if the file exists according to mode

    // if mode is read, and the file doesn’t exist
	// if mode is append/writing, and the file doesn’t exist 
    // create a new inode
    // create a new FILE, set its mode, and add it to the open_list
}