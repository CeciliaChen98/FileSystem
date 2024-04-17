#ifndef __FILESYSTEM_H_
#define __FILESYSTEM_H_

#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define max_num 20

struct Superblock{
    int size; 		/* size of data block: 512 */
    int inode_offset; 	/* offset of inode region */
    int data_offset; 		/* offset of data block region */
    int free_inode; 		/* Head of free inode list */
    int free_block; 		/*Head of free block list */
};

struct inode{
	int type; 		/* tell if it is a directory or a file */
	int permissions;		/* 4 bytes */
    char name[255];	/* the name of the file or directory */
	struct inode* parent;	/* if the current vnode is not in use, 
    we will use parent to point to the next free inode */
  	int nlink; 		/* number of links to this file */
    int size; 		/* numer of bytes in file */
   	int mtime; 		/* modification time */
   	int dblocks[N_DBLOCKS]; /* pointers to data blocks */
   	int iblocks[N_IBLOCKS]; /* pointers to indirect blocks */
   	int i2block; 		/* pointer to doubly indirect block */
};

typedef struct open_file{
	int index; 	/* assigned index when open */
    int inode; 	/* index of inode */
    int block_index; /* index of the block inside inode */
    int position;	/* offset inside the data block */
    int  mode;	/* read/write */
}File;

struct dirent{
	int inode;
	int type;
	int offset; 	/* stores the index of current sub-directory */
	char name[255];
};

struct Superblock sb;
File  file_table[max_num];
struct dirent* current_direct;
struct dirent* root_direct;

void disk_initialize(char* diskname){

}

File* f_open(char* filename, int mode){
	// do the path standardizing and permission check
    // check if the file exists according to mode
    // if mode is read, and the file doesn’t exist
	// if mode is append/writing, and the file doesn’t exist 
    // create a new inode
    // create a new FILE, set its mode, and add it to the open_list
}


int f_read(File *file, void* buffer, int num){
	// check the permission of FILE, if not permitted to read, return -1
	// get to the current position of the file according to block_index and position
	// if it is error
	
	// update block_index and position
}

int f_write(File* file, void* buffer, int num){
	// check the permission of FILE, if it is not allowed, return -1
	// get to the current position of the file according to block_index and position
	// if it is error
	
	// update block_index and position
}

int f_close(File* file){
    // delete the file from file_table
    // if it is not existing, return 0; else return 1
}

int f_seek(File* file, int num){
    // move according to num, block_index, and position
    // if failed, return 0; else, return 1
}

int f_rewind(File* file){
	// if the file is not existing, return 0
}

void f_stat(File* file){
	// print out the information of FILE file
}

int  f_delete(File* file){
	// update the data blocks, set the index of the data block to the head of free data block
	// clear inode & vnode, update free_inode & free_vnode
}

struct dirent* f_opendir(char* directory){
	// do the path standardizing and permission check
    // find the dirent of target directory and return it
    // if the directory is not existing
    // if the directory is not readable (by permission)
}

struct dirent* f_readdir(struct dirent* directory){
	// read the current sub-directory according to the offset
	// update the offset;
	// if the directory is not readable(by permission), return NULL;
}

int f_closedir(struct dirent* directory) {
	// if directory doesn’t exists
}

int  f_mkdir(char* path_name){
    // if directory is not existing

    // find the dirent according to the path_name
    // if there is already a sub-directory within the same name 
	// create a new struct dirent, store it into the data block of its parent directory
}

int f_rmdir(char* path_name) {
	// if path_name is not existing

    // find its parent directory and find the desired dirent
    // delete all the files and directories inside the dirent, and remove the dirent from the direct_list
    //delete the dirent from its parent’s data block
}

#endif