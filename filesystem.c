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

static int inode_num;
static int data_num;
static struct User* current_user;

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
    return (char*)block_data + block_size * index; 
}

int f_changeMod(int inode, int permission) {
    struct inode* changeInode = getInode(inode);
    int creatorid = changeInode->uid;
    if (creatorid != current_user->uid) {
        printf("You do not have permission to change the mode of this file\n");
        return -1;
    }
    if (changeInode == NULL || (permission < 0 && permission > 7)) {
        printf("Invalid inode or permission\n");
        return -1;
    } else {
        changeInode->permissions = permission;
        printf("The permission of the inode %d is now %d\n", inode, permission);
        return 0;
    }
}


void freeBlock(int blockindex) {
    memset(getData(blockindex),0,block_size);
    // Retrieve the first block in the free list.
    int first_free_block = sb->free_block;
    if (first_free_block == -1) {
        // If the free list is currently empty, just set this block as the first free block
        // This case may not be typically allowed based on your policy but is here for completeness
        sb->free_block = blockindex;
        // Assuming blocknum needs to point to 0 to indicate no next block
        int* block_data = (int*)getData(blockindex);
        
        *block_data = -1; // Set the next pointer to 0
        return;
    }

    // Get the data from the first free block, which contains the pointer to the next free block
    int* first_block_data = (int*)getData(first_free_block);
    int second_free_block = *first_block_data;

    // Set the new block's next pointer to the current second block
    int* new_block_data = (int*)getData(blockindex);
    *new_block_data = second_free_block;

    // Update the first block's pointer to point to the new block
    *first_block_data = blockindex;
}

static void freeTokenizer(struct Tokenizer* tokenizer) {
    if (tokenizer == NULL) {
        return; // Nothing to free
    }

    // Free memory for each token
    for (int i = 0; i < tokenizer->length; i++) {
        free(tokenizer->tokens[i]);
    }

    // Free memory for the Tokenizer structure itself
    free(tokenizer);
}

static void cleaninode(struct inode* inode, int all) {
    // Assuming there is a function to free blocks and add them to free list
    // void freeBlock(int blocknum);

    // Free direct data blocks
    for (int i = 0; i < N_DBLOCKS; i++) {
        if (inode->dblocks[i] != 0) { // Check if the block is in use
            freeBlock(inode->dblocks[i]); // Free the block
            inode->dblocks[i] = 0; // Reset the pointer in the inode
        }
    }

    // Free indirect data blocks
    for (int i = 0; i < N_IBLOCKS; i++) {
        if (inode->iblocks[i] != 0) { // Check if the block is in use
            int* block_data = (int*)getData(inode->iblocks[i]); // Get the block containing pointers
            for (int j = 0; j < sb->size / sizeof(int); j++) {
                if (block_data[j] != 0) {
                    freeBlock(block_data[j]); // Free each data block pointed by the indirect block
                    block_data[j] = 0; // Reset the pointer in the data block
                }
            }
            freeBlock(inode->iblocks[i]); // Finally, free the indirect block itself
            inode->iblocks[i] = 0; // Reset the indirect block pointer in the inode
        }
    }

    // Free doubly indirect block
    if (inode->i2block != 0) {
        int* first_level_block = (int*)getData(inode->i2block); // Get first level block
        for (int i = 0; i < sb->size / sizeof(int); i++) {
            if (first_level_block[i] != 0) {
                int* second_level_block = (int*)getData(first_level_block[i]); // Get second level block
                for (int j = 0; j < sb->size / sizeof(int); j++) {
                    if (second_level_block[j] != 0) {
                        freeBlock(second_level_block[j]); // Free each block pointed by second level block
                        second_level_block[j] = 0; // Reset pointer
                    }
                }
                freeBlock(first_level_block[i]); // Free second level block itself
                first_level_block[i] = 0; // Reset pointer in first level block
            }
        }
        freeBlock(inode->i2block); // Free the first level doubly indirect block
        inode->i2block = 0; // Reset the doubly indirect block pointer in the inode
    }

    if(all==1){
        inode->permissions = NONE;
        int next_free = getInode(sb->free_inode)->parent;
        getInode(sb->free_inode)->parent = inode->index; 
        inode->parent = next_free;
        inode->type = 0;
        inode->mtime = 0;
        inode->size = 0;
        inode->nlink = 0;
        return;

    }
    // Additionally, reset inode metadata (optional based on requirement)
    inode->size = 0;         // Reset file size
    inode->mtime = time(NULL);        // Reset modification time
    inode->nlink = 0;        // Reset number of links
}

static struct dirent* findAllByIndex(struct inode inode, int INDEX) {
    int read_num = 0;
    int count = 0;

    if(INDEX>inode.size/sizeof(struct dirent)-2){return NULL;}
    // Direct blocks
    for (int i = 0; i < N_DBLOCKS && read_num < inode.size; i++) {
        char* data = getData(inode.dblocks[i]);
        for (int block_num = 0; block_num < block_size; block_num += sizeof(struct dirent)) {
            if (read_num >= inode.size) return NULL;

            struct dirent* cur_dirent = (struct dirent*)(data + block_num);
            if (strcmp(cur_dirent->name, ".") != 0&& strcmp(cur_dirent->name, "..") != 0) {
                if (count == INDEX) {
                    return cur_dirent;
                }
                count++;
            }
            read_num += sizeof(struct dirent);
        }
    }

    // Indirect blocks
    for (int i = 0; i < N_IBLOCKS && read_num < inode.size; i++) {
        int* index_data = (int*)getData(inode.iblocks[i]);
        for (int index = 0; index < block_size / sizeof(int); index++) {
            char* data = getData(index_data[index]);
            for (int block_num = 0; block_num < block_size; block_num += sizeof(struct dirent)) {
                if (read_num >= inode.size) return NULL;

                struct dirent* cur_dirent = (struct dirent*)(data + block_num);
                    if (count == INDEX) {
                        return cur_dirent;
                    }
                    count++;
                read_num += sizeof(struct dirent);
            }
        }
    }

    // Doubly indirect blocks
    int* doubly_indirect = (int*)getData(inode.i2block);
    for (int d_index = 0; d_index < block_size / sizeof(int) && read_num < inode.size; d_index++) {
        int* index_data = (int*)getData(doubly_indirect[d_index]);
        for (int index = 0; index < block_size / sizeof(int); index++) {
            char* data = getData(index_data[index]);
            for (int block_num = 0; block_num < block_size; block_num += sizeof(struct dirent)) {
                if (read_num >= inode.size) return NULL;

                struct dirent* cur_dirent = (struct dirent*)(data + block_num);
                    if (count == INDEX) {
                        return cur_dirent;
                    }
                    count++;
                read_num += sizeof(struct dirent);
            }
        }
    }

    return NULL;  // If no dirent was found at the specified index
}

static int assignBlock(){

    if(sb->free_block==-1){printf("No more memory\n");return -1;}
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
    // clean the data inside the new data block
    char* new_data_block = getData(data_file);
    memset(new_data_block,0,block_size);

    return data_file;
}

static void* appendPosition(struct inode* inode,int block_index, int position, int request){
    // request: write = 1, read = 0
    // calculate the offset
    int last_block = inode->size/block_size;
    int num_index = block_size/sizeof(int);
    int calculated_size = block_index*block_size+position;
    // if it exceeds the size limit
    if(block_index>(N_DBLOCKS+N_IBLOCKS*num_index+num_index*num_index)){return NULL;}
    // if it reach an unassigned position
    if(calculated_size>=inode->size){ 
        if(request==0){
            return NULL; 
        }else{
            if(block_index==last_block&&position!=0){
                if(block_index<N_DBLOCKS){
                    return getData(inode->dblocks[block_index])+position;
                }
                block_index = block_index - N_DBLOCKS;
                if(block_index<N_IBLOCKS*num_index){
                    int a = block_index/num_index;
                    int b = block_index%num_index;
                    return getData(*((int*)getData(inode->iblocks[a])+b))+position;
                }
                block_index = block_index - N_IBLOCKS*num_index;
                if(block_index<num_index*num_index){
                    int a = block_index/num_index;
                    int b = block_index%num_index;
                    int index = *((int*)getData(inode->i2block)+a);
                    return getData(*((int*)getData(index)+b))+position;
                }else{
                    return NULL;
                }
            }else{
                if(block_index==last_block){last_block--;}
                // assign block in betweem last_block and block_index
                if(block_index < N_DBLOCKS){
                    int index = 0;
                    for(int i=last_block+1;i<=block_index;i++){
                        index = assignBlock();
                        inode->dblocks[i] = index;
                    }
                    return getData(index)+ position;
                }else{
                    for(int i=last_block+1;i<N_DBLOCKS;i++){
                        last_block++;
                        inode->dblocks[i] = assignBlock();
                    }
                }
                block_index = block_index - N_DBLOCKS;
                last_block = last_block - N_DBLOCKS;
            
                int c = last_block/num_index;
                int d = last_block%num_index;

                if(block_index<N_IBLOCKS*num_index){
                    int a = block_index/num_index;
                    int b = block_index%num_index;
                    int index;
                    for(int i=c;i<=a;i++){
                       int start = 0;
                       int end = num_index-1;
                       if(i==c){start=d+1;}
                       if(i==a){end=b;}
                       for(int j=start;j<=end;j++){
                            int* ptr = (int*)getData(inode->iblocks[i])+j;
                            index = assignBlock();
                            *ptr = index;
                        }    
                    }
                    return getData(index)+ position;
                }else{
                    for(int i=c;i<num_index;i++){
                        int start = 0;
                        if(i==c){start=d+1;}
                        for(int j=start;j<num_index;j++){
                            int* ptr = (int*)getData(inode->iblocks[i])+j;
                            *ptr = assignBlock();
                            last_block++;
                        }
                    }
                }
                block_index = block_index - N_IBLOCKS*num_index;
                last_block = last_block - N_IBLOCKS*num_index;
                c = last_block/num_index;
                d = last_block%num_index;
                if(block_index<num_index*num_index){
                    int a = block_index/num_index;
                    int b = block_index%num_index;
                    int index;
                    for(int i=c;i<=a;i++){
                       int start = 0;
                       int end = num_index-1;
                       if(i==c){start=d+1;}
                       if(i==a){end=b;}
                       for(int j=start;j<=end;j++){
                            int* ptr = (int*)getData(*((int*)getData(inode->i2block)+i))+j;
                            index = assignBlock();
                            *ptr = index;
                        }    
                    }
                    return getData(index)+ position;
                }
            }
        }
    }else{
        if(block_index<N_DBLOCKS){
            return getData(inode->dblocks[block_index])+position;
        }
        block_index = block_index - N_DBLOCKS;
        if(block_index<N_IBLOCKS*num_index){
            int a = block_index/num_index;
            int b = block_index%num_index;
            return getData(*((int*)getData(inode->iblocks[a])+b))+position;
        }
        block_index = block_index - N_IBLOCKS*num_index;
        if(block_index<num_index*num_index){
            int a = block_index/num_index;
            int b = block_index%num_index;
            int index = *((int*)getData(inode->i2block)+a);
            return getData(*((int*)getData(index)+b))+position;
        }else{
            return NULL;
        }
    }
    return NULL;
}

static struct dirent* createFile(struct dirent* cur_dirent, char* name){

    // we don't assign any data block for a new file
    // rearrange the free inode 
    if(sb->free_inode==-1){printf("No more memory\n"); return NULL;}
    int free_i = getInode(sb->free_inode)->parent;
    struct inode* file_inode;
    if(free_i!=-1){
        int next_i = getInode(free_i)->parent;
        file_inode = getInode(free_i);
        getInode(sb->free_inode)->parent = next_i;
    }else{
        free_i = sb->free_inode;
        file_inode = getInode(free_i);
        sb->free_inode = -1;
    }
    // modify the inode to store all the information
    file_inode -> type = FILE_TYPE;
    file_inode -> permissions = RWx;
    file_inode -> uid = current_user->uid;
    file_inode -> parent = cur_dirent->inode;
    file_inode -> nlink = 0;
    file_inode -> size = 0;
    file_inode -> mtime = time(NULL);    

    struct inode* inode = getInode(cur_dirent->inode);;
    struct dirent* direct = (struct dirent*)appendPosition(inode,inode->size/block_size,inode->size%block_size,1);
    inode->size += sizeof(struct dirent);
    if(direct==NULL){return NULL;}
    direct->inode = free_i; 
    direct->type = FILE_TYPE;
    direct->offset = -1;
    strcpy(direct->name,name);

    return direct;

}

static struct dirent* findDirent(struct inode inode, char* target) {
    int read_num = 0;

    // Loop through direct blocks
    for (int i = 0; i < N_DBLOCKS; i++) {
        char* data = getData(inode.dblocks[i]);
        int block_num = 0;
        while (block_num < block_size) {
            struct dirent* cur_dirent = (struct dirent*)data;
            if (strcmp(target, cur_dirent->name) == 0) {
                return cur_dirent;
            }
            read_num += sizeof(struct dirent);
            block_num += sizeof(struct dirent);
            if (read_num >= inode.size) {
                return NULL;
            }
            data += sizeof(struct dirent);
        }
    }

    // Loop through indirect blocks
    for (int i = 0; i < N_IBLOCKS; i++) {
        int* index_data = (int*)getData(inode.iblocks[i]);
        for (int index = 0; index < block_size / sizeof(int); index++) {
            char* data = getData(index_data[index]);
            int block_num = 0;
            while (block_num < block_size) {
                struct dirent* cur_dirent = (struct dirent*)data;
                if (strcmp(target, cur_dirent->name) == 0 ) {
                    return cur_dirent;
                }
                read_num += sizeof(struct dirent);
                block_num += sizeof(struct dirent);
                if (read_num >= inode.size) {
                    return NULL;
                }
                data += sizeof(struct dirent);
            }
        }
    }

    // Loop through doubly indirect blocks
    int* doubly_indirect = (int*)getData(inode.i2block);
    for (int d_index = 0; d_index < block_size / sizeof(int); d_index++) {
        int* index_data = (int*)getData(doubly_indirect[d_index]);
        for (int index = 0; index < block_size / sizeof(int); index++) {
            char* data = getData(index_data[index]);
            int block_num = 0;
            while (block_num < block_size) {
                struct dirent* cur_dirent = (struct dirent*)data;
                if (strcmp(target, cur_dirent->name) == 0) {
                    return cur_dirent;
                }
                read_num += sizeof(struct dirent);
                block_num += sizeof(struct dirent);
                if (read_num >= inode.size) {
                    return NULL;
                }
                data += sizeof(struct dirent);
            }
        }
    }

    return NULL;  // If no dirent was found
}

static struct dirent* findDirentByInode(struct inode inode, int target) {
    int read_num = 0;

    // Loop through direct blocks
    for (int i = 0; i < N_DBLOCKS; i++) {
        char* data = getData(inode.dblocks[i]);
        int block_num = 0;
        while (block_num < block_size) {
            struct dirent* cur_dirent = (struct dirent*)data;
            if (target== cur_dirent->inode&& cur_dirent->type == DIRECTORY_TYPE) {
                return cur_dirent;
            }
            read_num += sizeof(struct dirent);
            block_num += sizeof(struct dirent);
            if (read_num >= inode.size) {
                return NULL;
            }
            data += sizeof(struct dirent);
        }
    }

    // Loop through indirect blocks
    for (int i = 0; i < N_IBLOCKS; i++) {
        int* index_data = (int*)getData(inode.iblocks[i]);
        for (int index = 0; index < block_size / sizeof(int); index++) {
            char* data = getData(index_data[index]);
            int block_num = 0;
            while (block_num < block_size) {
                struct dirent* cur_dirent = (struct dirent*)data;
                if (target== cur_dirent->inode&& cur_dirent->type == DIRECTORY_TYPE) {
                    return cur_dirent;
                }
                read_num += sizeof(struct dirent);
                block_num += sizeof(struct dirent);
                if (read_num >= inode.size) {
                    return NULL;
                }
                data += sizeof(struct dirent);
            }
        }
    }

    // Loop through doubly indirect blocks
    int* doubly_indirect = (int*)getData(inode.i2block);
    for (int d_index = 0; d_index < block_size / sizeof(int); d_index++) {
        int* index_data = (int*)getData(doubly_indirect[d_index]);
        for (int index = 0; index < block_size / sizeof(int); index++) {
            char* data = getData(index_data[index]);
            int block_num = 0;
            while (block_num < block_size) {
                struct dirent* cur_dirent = (struct dirent*)data;
                if (target== cur_dirent->inode&& cur_dirent->type == DIRECTORY_TYPE) {
                    return cur_dirent;
                }
                read_num += sizeof(struct dirent);
                block_num += sizeof(struct dirent);
                if (read_num >= inode.size) {
                    return NULL;
                }
                data += sizeof(struct dirent);
            }
        }
    }

    return NULL;  // If no dirent was found
}


static struct Tokenizer* tokenize(char* arg){
    if(arg==NULL){
        return NULL;
    }
    // Initialize a tokenizer
    struct Tokenizer* path = malloc(sizeof(struct Tokenizer));
    if (path == NULL) {
        return NULL; // Failed to allocate memory for Tokenizer
    }
    int i = 0;
    if(arg[i]=='/' || arg[i]=='~'){
        path->flag = PATH_ROOT;
        i++;
    } else {
        path->flag = PATH_CURRENT;
    }

    char* arg_copy = strdup(arg + i);
    if (arg_copy == NULL) {
        free(path);
        return NULL; // Failed to allocate memory for arg_copy
    }

    char* token = strtok(arg_copy, "/");
    int arg_count = 0;
    // Tokenize input
    while (token != NULL) {
        if(arg_count >= MAX_INPUT_SIZE){
            free(arg_copy);
            free(path);
            return NULL; // Reached maximum input size
        }
        path->tokens[arg_count] = strdup(token); // Allocate memory and copy token string
        if (path->tokens[arg_count] == NULL) {
            // Failed to allocate memory for token string
            // Clean up allocated memory before returning NULL
            for (int j = 0; j < arg_count; j++) {
                free(path->tokens[j]);
            }
            free(arg_copy);
            free(path);
            return NULL;
        }
        arg_count++;
        token = strtok(NULL, "/");
    } 
    free(arg_copy); // Free the memory allocated for arg_copy
    path->length = arg_count; 
    return path;
}

// Function to authenticate user directly from data blocks
int f_userAuthen(char* username, char* password) {
    for (int i = 0; i < N_DBLOCKS; i++) {
        if (user_inode->dblocks[i] != 0 || (i == 0 && user_inode->dblocks[i] == 0)) {
            char* data = getData(user_inode->dblocks[i]);
            if (!data) continue; // Handle NULL data

            for (int j = 0; j < block_size; j += sizeof(struct User)) {
                struct User* user = (struct User*)(data + j);
                if (user->uid == -1) {
                    break;  // Stop processing this block if sentinel value found
                }

                // Perform authentication check directly
                if (strcmp(user->username, username) == 0 && strcmp(user->password, password) == 0) {
                    printf("User %s has been authenticated\n", username);
                    strcpy(current_user->username, username);
                    strcpy(current_user->password, password);
                    current_user->uid = user->uid;
                    return 0; // Authentication successful
                }
            }
        }
    }
    return -1; // Authentication failed
}


int disk_open(char *diskname){
    // open the disk image
	FILE* file = fopen(diskname,"r+b");
    if (file == NULL) {
        printf("Disk not found. Please run ./format diskimage to create a disk.\n");
        return 0;
    }
    diskimage = file;

    sb = (struct Superblock*)malloc(sizeof(struct Superblock));
    if(fread(sb,sizeof(struct Superblock),1,diskimage) == 0){
        printf("Can't read superblock\n");
        free(sb);
        fclose(diskimage);
        return 0;
    }

    // read the user inode
    user_inode = (struct inode*)malloc(sizeof(struct inode));
    if(fread(user_inode,sizeof(struct inode),1,diskimage) <1){
        printf("Can't read user inode\n");
        free(sb);free(user_inode);
        fclose(diskimage);
        return 0;
    }
    current_user = malloc(sizeof(struct User));

    if (fseek(diskimage, 512, SEEK_SET) != 0) {
        perror("Error seeking file");
        free(sb); free(user_inode); free(current_user);
        fclose(diskimage); // Close the file
        return -1; // Exit program with an error
    }
    
    block_size = sb->size;
    inode_num = block_size*(sb->data_offset-sb->inode_offset);
    inode_data = (struct inode*)malloc(inode_num);
    
    if(fread(inode_data,inode_num,1,diskimage)<1){
        printf("Can't read inode region\n");
        return 0;
    }
    root_direct = malloc(sizeof(struct dirent));
    root_direct->inode = 0;
    root_direct->offset = -1;
    root_direct->type = DIRECTORY_TYPE;
    strcpy(root_direct->name,"root");
    
    //printf("inode %d\n",inode_data->size);
    current_direct = malloc(sizeof(struct dirent));
    current_direct->inode = 0;
    current_direct->type = DIRECTORY_TYPE;
    strcpy(current_direct->name,"root");
    
    if (fseek(diskimage, 0, SEEK_END) != 0) {
        perror("Error seeking file");
        fclose(diskimage); // Close the file
        return -1; // Exit program with an error
    }
    
    int num_bytes = ftell(diskimage);
    if (num_bytes == -1) {
        perror("Error getting file size");
        fclose(diskimage); // Close the file
        return 1; // Exit program with an error
    }
    fseek(diskimage,512+(sb->data_offset*512),SEEK_SET);
    //printf("num of bytes %d\n",num_bytes); //好像少了128bytes？

    data_num = num_bytes-512-inode_num;
    block_data = (char*)malloc(data_num);
    if(fread(block_data,data_num,1,diskimage)==0){
        printf("Can't read data block region\n");
        return 0;
    }
    //printf("%s\n",((struct dirent*)block_data+2)->name);
    return 1;
}

int disk_close(){
    free(current_direct);
    free(root_direct);
    free(current_user);
    fseek(diskimage,0,SEEK_SET);
    if(fwrite(sb,sizeof(struct Superblock),1,diskimage)<1){
        free(sb);
        free(user_inode);
        free(inode_data);
        free(block_data);
        fclose(diskimage);
        return 0;
    }
    free(sb);
    if(fwrite(user_inode,sizeof(struct inode),1,diskimage)<1){
        free(user_inode);
        free(inode_data);
        free(block_data);
        fclose(diskimage);
        return 0;
    }
    free(user_inode);
    fseek(diskimage,512,SEEK_SET);
    if(fwrite(inode_data,inode_num,1,diskimage)<1){
        free(inode_data);
        free(block_data);
        fclose(diskimage);
        return 0;
    }
    free(inode_data);
    if(fwrite(block_data,data_num,1,diskimage)<1){
        free(block_data);
        fclose(diskimage);
        return 0;
    }
    free(block_data);
    fclose(diskimage);
    return 1;
} 

File* f_open(char* filename, char* mode){

    if(filename[strlen(filename)-1]=='/'){
        return NULL;
    }
	// do the path standardizing and permission check
    struct Tokenizer* path = tokenize(filename);
    // if the path is invalid (exceeds MAX_INPUT_SIZE)
    if(path==NULL){
        return NULL;
    }
    // check if there is a directory needed to open
    struct dirent* target = current_direct;
    if(path->length>1){
        int len = strlen(filename)-strlen(path->tokens[path->length-1]);
        char* pathname = malloc(len+1);
        for (int i = 0; i < len; i++) {
            pathname[i] = filename[i];
        }
        pathname[len] = '\0';
        target = f_opendir(pathname);
        free(pathname);
    }
    if(target == NULL){freeTokenizer(path);return NULL;}
    char* name = NULL;
    name = path->tokens[path->length-1];
    if(strcmp(name,"..")==0||strcmp(name,".")==0){freeTokenizer(path);return NULL;}
    struct inode* temp_inode = getInode(target->inode);
    struct dirent* target_file = findDirent(*temp_inode,name);
    int permission = NONE;
    if(target_file!=NULL){permission = getInode(target_file->inode)->permissions;}
    
    if(target_file!=NULL){if(target_file->type==DIRECTORY_TYPE){printf("cannot open '%s': is a directory\n",name); return NULL;}}
    // mode
    int int_mode = -1;
    if(strcmp("r",mode)==0){
        if(target_file==NULL){
            printf("%s:No such file or directory\n",filename);
            freeTokenizer(path);
            return NULL;
        }
        if(current_user->uid ==0){
            //superuser
        }else if(permission!=Rwx&&permission!=RWx&&permission!=RWX&&permission!=RwX){
            printf("Wrong permission\n");
            freeTokenizer(path);
            return NULL;
        }
        int_mode = READ;
    }
    else if(strcmp("r+",mode)==0){
        if(target_file==NULL){
            printf("%s:No such file or directory\n",filename);
            freeTokenizer(path);
            return NULL;
        }
        if(current_user->uid ==0){
            //superuser
        }else if(permission!=RWX&&permission!=RWx){
            printf("Wrong permission\n");
            freeTokenizer(path);
            return NULL;
        }
        int_mode = READ_WRITE;
    }
    else if(strcmp("w",mode)==0){int_mode = WRITE;}
    else if(strcmp("w+",mode)==0){int_mode = WRITE_READ;}
    else if(strcmp("a",mode)==0){int_mode = APPEND;}
    else if(strcmp("a+",mode)==0){int_mode = APPEND_READ;}
    else{
        printf("Invalid request\n");
        freeTokenizer(path);
        return NULL;
    }
    File* file = (File*)malloc(sizeof(File));
    file->block_index = 0;
    file->position = 0;

    if(int_mode==READ || int_mode==READ_WRITE){
        if(target_file==NULL){
            printf("%s:No such file or directory\n",filename);
            free(file);
            freeTokenizer(path);
            return NULL;
        }else{
            file->inode = target_file->inode;
        }
    }
    else if(int_mode==WRITE || int_mode== WRITE_READ){
        if(target_file==NULL){
            file->inode = createFile(target,name)->inode;
        }else{
            file->inode = target_file->inode;
            cleaninode(getInode(target_file->inode),0);
        }
    }
    else if(int_mode==APPEND || int_mode== APPEND_READ){
        if(target_file==NULL){
            file->inode = createFile(target,name)->inode;
        }else{
            if(current_user->uid==0||   //superuser
            (int_mode==APPEND&&(permission==rWx||permission==rWX||permission==RWX||permission==RWx))||
            (int_mode==APPEND_READ&&(permission==RWx||permission==RWX))){
                file->inode = target_file->inode;
                int size = getInode(file->inode)->size;
                file->block_index = size / block_size;
                file->position = size % block_size;
            }else{
                free(file);
                printf("Wrong Permission\n");
                freeTokenizer(path);
                return NULL;
            }
        }
    }
    getInode(file->inode)->nlink++;
    file->mode = int_mode;
    strcpy(file->name,filename);
    freeTokenizer(path);
    
    return file;
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
            printf("%s: Not a directory",directory);
            return NULL;
        }
        if (cur->type == FILE_TYPE){
            printf("%s: Not a directory",directory);
            return NULL;
        }
        // Perform directory matching or traversal
        if (strcmp(cur->name, path->tokens[count]) == 0) {
            // If the directory name matches the current token, continue to next token
            continue;
        }
        // Move to the next directory in the path
        cur = findDirent(inode_data[cur->inode], path->tokens[count]);
    }
    freeTokenizer(path);
    if(cur == NULL){
        printf("%s: Not a directory",directory);
        return NULL;
    }else if (cur->type == FILE_TYPE){
        printf("%s: Not a directory",directory);
    }
    cur->offset = 0;
    // Return the final directory entry found, or NULL if not found
    return cur;
}

int f_close(File* file){
    if(file==NULL){return 0;}
    //getInode(file->inode)->nlink--;
    free(file);
    return 1;
}

int f_rewind(File* file){
    if(file==NULL){return 0;}
    file->position = 0;
    file->block_index = 0;
    return 1;
}

struct dirent* f_readdir(struct dirent* directory){
	// read the current sub-directory according to the offset
    if(directory->offset==-1){
        return NULL;
    }
	// update the offset;
    struct dirent* cur = findAllByIndex(*getInode(directory->inode), directory->offset);
    if (cur != NULL) {
        directory->offset ++;
    }else{
        directory->offset = 0;
    }
    return cur;
}

int f_closedir(struct dirent* directory) {
	// if directory doesn’t exists
    if (directory == NULL) {
        return -1;
    }
    // remove the dirent from the open file table
	// set the offset of the directory to 0
    directory->offset = -1;
	return 0;
}

static void path_recur(char* path,struct inode* inode){
    
    //printf("%s\n",findDirentByInode(*getInode(0),3)->name);
    if(inode->parent!=-1){
        path_recur(path,getInode(inode->parent));
        strcat(path,"/");
        strcat(path,findDirentByInode(*getInode(inode->parent), inode->index)->name);
    }else{
        strcat(path,"/root");
    }   
}

void f_path(char* path) {
    path_recur(path, getInode(current_direct->inode));
}

void ls_helper(char* content,struct dirent* direct, int flag){
    struct inode* temp = getInode(direct->inode);
    if(flag==0){
        strcat(content,direct->name);
        strcat(content,"    ");
    }else if(flag==1){
        char tempStr[40];
        if(temp->permissions==0){strcat(content,"--- ");}
        else if(temp->permissions==Rwx){strcat(content,"r-- ");}
        else if(temp->permissions==RWx){strcat(content,"rw- ");}
        else if(temp->permissions==RwX){strcat(content,"r-x ");}
        else if(temp->permissions==RWX){strcat(content,"rwx ");}
        else if(temp->permissions==rWx){strcat(content,"-w- ");}
        else if(temp->permissions==rWX){strcat(content,"-wx ");}
        else if(temp->permissions==rwX){strcat(content,"--x ");}
        sprintf(tempStr, " %-3d %-3d %-10d ", temp->nlink,temp->uid,temp->size);
        //strcat(content,temp->mtime);
        strcat(content,tempStr);
        strcat(content,direct->name);
        strcat(content,"\n");
    }else{
        strcat(content,direct->name);
        if(temp->permissions%2==1){strcat(content,"*");}
        strcat(content,"    ");
    }
}

static void clearDirect(struct dirent* direct){
    struct inode* inode = getInode(direct->inode);
    //printf("Here: inode %d\n",inode->index);
    if(direct->type==FILE_TYPE){
        //printf("Clean File\n");
        cleaninode(inode,1);
    }
    else if(inode->size==256){
        //printf("Clean Directory\n");
        cleaninode(inode,1);
    }else{
        for(int i=0;i<(inode->size/sizeof(struct dirent)-2);i++){
            struct dirent* direct = findAllByIndex(*inode,i);
            //printf("i:%d, directname:%s\n",i,direct->name);
            clearDirect(direct);
        }
        cleaninode(inode,1);
    }
}

int f_rmdir(char* path_name, int flag) {
    // flag = 0: only remove direnctory that is empty
    if (path_name == NULL) {
        printf("Invalid path name\n");
        return -1;
    }

    struct Tokenizer* path = tokenize(path_name);
    if (path == NULL) {
        return -1;  // Failed to tokenize the path
    }

    struct dirent* cur = (path->flag == PATH_CURRENT) ? current_direct : root_direct;

    for (int count = 0; count < path->length; count++) {
        if (cur == NULL) {
            printf("failed to remove '%s': No such file or directory\n",path_name);
            freeTokenizer(path);
            return -1;
        }

        if (count == path->length - 1) {  // Last token, should be the directory to delete
            if (strcmp(cur->name, path->tokens[count]) != 0) {
                cur = findDirent(inode_data[cur->inode], path->tokens[count]);
                if (cur == NULL) {
                    printf("failed to remove '%s': No such file or directory\n",path_name);
                    freeTokenizer(path);
                    return -1;
                }
                if (cur->type==FILE_TYPE){
                    printf("failed to remove '%s': Not a directory\n",cur->name);
                    freeTokenizer(path);
                    return -1;
                }
            }
        } else {
            cur = findDirent(inode_data[cur->inode], path->tokens[count]);
            if (cur->type==FILE_TYPE){
                printf("failed to remove '%s': No adirectory\n",cur->name);
                freeTokenizer(path);
                return -1;
            }
        }
    }

    freeTokenizer(path);

    // At this point, 'cur' should be the directory to remove
    struct inode* curinode = getInode(cur->inode);
    struct inode* parent = getInode(curinode->parent);
    // check permissioin
    if(current_user->uid!=0&&current_user->uid!=curinode->uid){
        printf("cannot remove '%s': Permission denied\n",cur->name);
        return -1;
    }
    if(flag==1){
        clearDirect(cur);
    }else{
        if (curinode->size != 2*sizeof(struct dirent)) {
            printf("failed to remove '%s': Directory not empty\n",cur->name);
            return -1;
        }
        cleaninode(curinode,1);
    }
    
    // move the last dirent to the current position
    int refer_index = parent->size - sizeof(struct dirent);
    int temp_index = refer_index/block_size;
    struct dirent* last = (struct dirent*)appendPosition(parent,temp_index,refer_index%block_size,0);
    if(cur==last){
    }else{
         // set the dirent all to 0
        cur->inode = last->inode;
        strcpy(cur->name,last->name);  
        cur->offset = last->offset;
        cur->type = last->type;
    }
    
    int tst = ((char*)last-block_data)/block_size;
    if(refer_index%block_size==0){
        freeBlock(tst);
        if(temp_index<N_DBLOCKS){
            parent->dblocks[temp_index]=0;
        }else{
            temp_index = temp_index - N_DBLOCKS;
            int num_index = block_size/sizeof(int);
            if(temp_index<N_IBLOCKS*num_index){
                int a = temp_index/num_index;
                int b = temp_index%num_index;
                int* ptr = (int*)getData(parent->iblocks[a])+b;
                *ptr = 0;
            }else{  
                temp_index = temp_index - N_IBLOCKS*num_index;
                int a = temp_index/num_index;
                int b = temp_index%num_index;
                int* ptr = (int*)getData(*((int*)getData(parent->i2block)+a)+b);
                *ptr = 0;
            }
        }
    }else{
        memset(last,0,sizeof(struct dirent));
    }
    parent->size -= sizeof(struct dirent);
    return 0;
}

int f_write(File* file, void* buffer, int num){
    if(file==NULL){return -1;}

	// check the mode of FILE, if it is not allowed, return -1
    if(file->mode == READ){return -1;}
    char* char_buffer = (char*) buffer;

    if(num>strlen(char_buffer)+1){num = strlen(char_buffer)+1;}
    struct inode* inode = getInode(file->inode);

    int written_num = 0;

    // write content
    while(written_num<num){
        int avail_bytes = block_size - file->position;
        //printf("avail_bytes:%d\n",avail_bytes);
        // get to the current position of the file according to block_index and position
        char* data = appendPosition(inode,file->block_index,file->position,1);
	    // if there is no more space
	    if(data==NULL){return written_num;}

        // if remained data can't fill the whole data_block
        if(avail_bytes>(num-written_num)){
            // update position
            avail_bytes=num-written_num;
            file->position+=avail_bytes;
            
            memcpy(data,char_buffer+written_num,avail_bytes);
            // update file size
            inode->size += avail_bytes;
            written_num += avail_bytes;
            return written_num;
        }else{
            // update position
            file->position = 0;
            file->block_index++;
        }
        memcpy(data,(char*)buffer+written_num,avail_bytes);
        // update file size
        inode->size += avail_bytes;
        written_num += avail_bytes;
    }
    return written_num;
    
}

int f_read(File *file, void* buffer, int num){
	
    if(file==NULL){return -1;}

	// check the mode of FILE, if it is not allowed, return -1
    if(file->mode == APPEND || file->mode == WRITE){return -1;}
    struct inode* inode = getInode(file->inode);
	int read_num = 0;
    int remained_bytes = inode->size-(file->block_index*block_size+file->position);
    // read content
    while(read_num<num){
        int need_bytes = num - read_num;
        int flag = 0;
        // get to the current position of the file according to block_index and position
        char* data = appendPosition(inode,file->block_index,file->position,0);
	    // if there is no more data to read
	    if(data==NULL){return read_num;}
        if(need_bytes>(block_size-file->position)){
            flag = 1;
            need_bytes = block_size-file->position;
        }
        // no enough bytes to read; the last time to read
        if(need_bytes>remained_bytes){
            need_bytes = remained_bytes;
            file->position+=need_bytes;
            if(need_bytes==0){return read_num;}
            memcpy((char*)buffer+read_num,data,need_bytes);
            read_num += need_bytes;
            return read_num;
        }
        if(flag==1){
            file->block_index ++;
            file->position = 0;
        }else{
            file->position+=need_bytes;
        }
        memcpy((char*)buffer+read_num,data,need_bytes);
        read_num += need_bytes;
        remained_bytes -= need_bytes;
        //printf("\n");
    }
    return read_num;
}

int f_stat(char* filename){
    if (filename == NULL) {
        printf("Invalid file name\n");
        return -1;
    }

    File* cur = f_open(filename, "r");
    // if cur is not a file (a directory or not exists)
    if (cur == NULL) {
        struct dirent* curdir = f_opendir(filename);
        if (curdir == NULL) {
            printf("file/directory not found.\n");
            return -1;
        }
        printf("File size: %d\n", getInode(curdir->inode)->size);
        printf("File type: directory\n");
        printf("Inode indexs: %d\n", curdir->inode);
        printf("Number of hard link: %d\n", getInode(curdir->inode)->nlink);
        printf("Permission: %d\n", getInode(curdir->inode)->permissions);
        f_closedir(curdir);
        return 0;
    }
    // if cur is a directory
    printf("File size: %d\n", getInode(cur->inode)->size);
    printf("File type: file\n");
    printf("Inode index: %d\n", cur->inode);
    printf("Number of hard link: %d\n", getInode(cur->inode)->nlink);
    printf("Permission: %d\n", getInode(cur->inode)->permissions);

    f_close(cur);

    return 0;
}

static struct dirent* createDirectory(struct dirent* cur_dirent, char* name){

    // we don't assign any data block for a new file
    // rearrange the free inode 
    if(sb->free_inode==-1){printf("No more memory\n"); return NULL;}
    int free_i = getInode(sb->free_inode)->parent;
    struct inode* file_inode;
    if(free_i!=-1){
        int next_i = getInode(free_i)->parent;
        file_inode = getInode(free_i);
        getInode(sb->free_inode)->parent = next_i;
    }else{
        free_i = sb->free_inode;
        file_inode = getInode(free_i);
        sb->free_inode = -1;
    }

    // modify the inode to store all the information
    file_inode -> type = DIRECTORY_TYPE;
    file_inode -> permissions = NONE;
    file_inode -> parent = cur_dirent->inode;
    file_inode -> nlink = 0;
    file_inode -> uid = current_user->uid;
    file_inode -> size = 0;
    file_inode -> mtime = time(NULL);    

    struct dirent* base = (struct dirent*)appendPosition(file_inode,0,0,1);
    base->inode = file_inode->index;
    strcpy(base->name,".");
    base->type = DIRECTORY_TYPE;
    base->offset = -1;
    file_inode -> size = 2*sizeof(struct dirent);

    base = base + 1;
    base->inode = cur_dirent->inode;
    strcpy(base->name,"..");
    base->type = DIRECTORY_TYPE;
    base->offset = -1;

    struct inode* inode = getInode(cur_dirent->inode);;
    struct dirent* direct = (struct dirent*)appendPosition(inode,inode->size/block_size,inode->size%block_size,1);
    inode->size += sizeof(struct dirent);
    if(direct==NULL){return NULL;}
    direct->inode = free_i; 
    direct->type = DIRECTORY_TYPE;
    direct->offset = -1;
    strcpy(direct->name,name);

    return direct;

}

struct dirent* f_mkdir(char* path_name){
    struct Tokenizer* path = tokenize(path_name);
    if(path==NULL){
        return NULL;
    }
    // check if there is a directory needed to open
    struct dirent* target = current_direct;
    if(path->length>1){
        int len = strlen(path_name)-strlen(path->tokens[path->length-1]);
        char* pathname = malloc(len+1);
        for (int i = 0; i < len; i++) {
            pathname[i] = path_name[i];
        }
        pathname[len] = '\0';
        target = f_opendir(pathname);
        free(pathname);
    }
    if(target == NULL){freeTokenizer(path);return NULL;}

    char* name = NULL;
    name = path->tokens[path->length-1];
    struct inode* temp_inode = getInode(target->inode);
    //check if there is a file with the same name
    if(findDirent(*temp_inode,name)!=NULL){
        printf("cannot create directory '%s': File exists\n",name);
        freeTokenizer(path);return NULL;
    }
    struct dirent* new_direct = createDirectory(target,name);
    freeTokenizer(path);
    return new_direct;
}

int f_seek(File* file, int num, int mode){
    int index = 0;
    //
    if(mode==SEEK_SET){
        index = num;
    }else if(mode==SEEK_CUR){
        index = block_size * file->block_index + file->position+ num;
    }else if(mode==SEEK_END){
        index = getInode(file->inode)->size - num;  
    }else{
        return 0;
    }
    if(index<0){return -1;}
    else{
        file->block_index = index/block_size;
        file->position = index%block_size;
    }
    return 1;
}

int f_remove(char* filename){
    if(filename[strlen(filename-1)]=='/'){
        return -1;
    }
	// do the path standardizing and permission check
    struct Tokenizer* path = tokenize(filename);
    // if the path is invalid (exceeds MAX_INPUT_SIZE)
    if(path==NULL){
        return -1;
    }
    // check if there is a directory needed to open
    struct dirent* target = current_direct;
    if(path->length>1){
        int len = strlen(filename)-strlen(path->tokens[path->length-1]);
        char* pathname = malloc(len+1);
        for (int i = 0; i < len; i++) {
            pathname[i] = filename[i];
        }
        pathname[len] = '\0';
        target = f_opendir(pathname);
        free(pathname);
    }
    if(target == NULL){freeTokenizer(path);return -1;}
    char* name = NULL;
    name = path->tokens[path->length-1];
    if(strcmp(name,"..")==0||strcmp(name,".")==0){freeTokenizer(path);return -1;}
    struct inode* temp_inode = getInode(target->inode);
    struct dirent* target_file = findDirent(*temp_inode,name);
    if(target_file ==NULL){
        printf("cannot remove '%s': No such file or directory\n",name);
        return 0;
    }
    if(target_file->type!=FILE_TYPE){
        printf("cannot remove '%s': Is a directory\n",name);
        return 0;
    }
    
    struct inode* target_inode = getInode(target_file->inode);
    freeTokenizer(path);
    
    // check permission  
    if(current_user->uid!=0&&current_user->uid!=target_inode->uid){
        printf("cannot remove '%s': Permission denied\n",name);
        return 0;
    }
    cleaninode(target_inode,1);
    
    // move the last dirent to the current position
    int refer_index = temp_inode->size - sizeof(struct dirent);
    int temp_index = refer_index/block_size;
    struct dirent* last = (struct dirent*)appendPosition(temp_inode,temp_index,refer_index%block_size,0);
    if(last==target_file){
    }else{
        // set the dirent all to 0
        target_file->inode = last->inode;
        strcpy(target_file->name,last->name);  
        target_file->offset = last->offset;
        target_file->type = last->type;
    }
    int tst = ((char*)last-block_data)/block_size;
    if(refer_index%block_size==0){
        freeBlock(tst);
        if(temp_index<N_DBLOCKS){
            temp_inode->dblocks[temp_index]=0;
        }else{
            temp_index = temp_index - N_DBLOCKS;
            int num_index = block_size/sizeof(int);
            if(temp_index<N_IBLOCKS*num_index){
                int a = temp_index/num_index;
                int b = temp_index%num_index;
                int* ptr = (int*)getData(temp_inode->iblocks[a])+b;
                *ptr = 0;
            }else{  
                temp_index = temp_index - N_IBLOCKS*num_index;
                int a = temp_index/num_index;
                int b = temp_index%num_index;
                int* ptr = (int*)getData(*((int*)getData(temp_inode->i2block)+a)+b);
                *ptr = 0;
            }
        }
    }else{
        memset(last,0,sizeof(struct dirent));
    }
    temp_inode->size -= sizeof(struct dirent);
    return 1;
}

