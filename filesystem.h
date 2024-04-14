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
}FILE;

typedef struct dirent{
	int inode;
	int type;
	int offset; 	/* stores the index of current sub-directory */
	char name[255];
};


FILE  file_table[max_num];
struct dirent* current_direct;

FILE*  f_open(char* filename, int mode){
	// do the path standardizing and permission check
    // check if the file exists according to mode
    // if mode is read, and the file doesn’t exist
	// if mode is append/writing, and the file doesn’t exist 
    // create a new inode
    // create a new FILE, set its mode, and add it to the open_list
}


int f_read(FILE *file, void* buffer, int num){
// check the permission of FILE, if not permitted to read, return -1
	// get to the current position of the file according to block_index and position
	// if it is error
	return -1; 
	// update block_index and position
}

int f_write(FILE* file, void* buffer, int num){
	// check the permission of FILE, if it is not allowed, return -1
	// get to the current position of the file according to block_index and position
	// if it is error
	return -1; 
	// update block_index and position
}

int f_close(FILE* file){
    // delete the file from file_table
    // if it is not existing, return 0; else return 1
}

int f_seek(FILE* file, int num){
    // move according to num, block_index, and position
    // if failed, return 0; else, return 1
}

int f_rewind(FILE* file){
	// if the file is not existing, return 0
}

#endif