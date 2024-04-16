#include "filesystem.h"
#include <string.h>

#define PATH_CURRENT 0
#define PATH_ROOT 1


struct tokenizer{
    int flag;
    char* token;
};



static struct tokenizer* tokenize(char* arg){
    // check if the input is valid
    if(strlen(arg)<1){
        return NULL;
    }
    // initialize a toeknizer
    struct tokenizer* path;
    int i = 0;
    if(arg[i]=='/'||arg[i]=='~'){
        path->flag = PATH_ROOT;
        i++;
    }else{
        path->flag = PATH_CURRENT;
    }
    path->token = strtok(arg+i,"/");
    return path;

}