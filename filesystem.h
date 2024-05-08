#ifndef __FILESYSTEM_H_
#define __FILESYSTEM_H_

#include <stdio.h>

#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define max_num 20
#define max_name 116
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// uppercase means CAN, lowercase means CANNOT
// read 4, write 2, execute 1
enum Permission {
    NONE = 0,
    Rwx= 4,
    rWx = 2,
    RWx = 6,
    rwX = 1,
    RwX = 5, // allow read and execute
    rWX = 3, 
    RWX = 7, // all allow
};

struct User{
    char username[64];
    int uid;
    char password[56];
};

struct Superblock{
    int size; 		/* size of data block: 512 */
    int inode_offset; 	/* offset of inode region */
    int data_offset; 		/* offset of data block region */
    int free_inode; 		/* Head of free inode list */
    int free_block; 		/*Head of free block list */
};

struct inode{
    int index;
	int type; 		/* tell if it is a directory or a file */
	int permissions;		/* 4 bytes */
	int parent;	/* if the current vnode is not in use, 
    we will use parent to point to the next free inode */
  	int nlink; 		/* number of links to this file */
    int uid;    /*user id*/
    int size; 		/* numer of bytes in file */
   	int mtime; 		/* modification time */
   	int dblocks[N_DBLOCKS]; /* pointers to data blocks */
   	int iblocks[N_IBLOCKS]; /* pointers to indirect blocks */
   	int i2block; 		/* pointer to doubly indirect block */
};

typedef struct open_file{
	char name[max_name];
    int inode; 	/* index of inode */
    int block_index; /* index of the block inside inode */
    int position;	/* offset inside the data block */
    int mode;	/* read/write */
    int count; 
}File;

struct dirent{
	int inode;
	int type;
	int offset; 	/* stores the index of current sub-directory */
	char name[max_name];
};

struct Superblock* sb;
struct inode* user_inode;
struct inode* inode_data; 
char* block_data;
int block_size;
FILE* diskimage; 
//File file_table[max_num];
struct dirent* current_direct;
struct dirent* root_direct;

int disk_open(char* diskname);

int disk_close( );

File* f_open(char* filename, char* mode);

int f_read(File *file, void* buffer, int num);

int f_write(File* file, void* buffer, int num);

int f_close(File* file);

int f_seek(File* file, int num, int mode);

int f_rewind(File* file);

void f_path(char* path);

int f_stat(char* filename);

int f_remove(char* filename);

struct dirent* f_opendir(char* directory);

struct dirent* f_readdir(struct dirent* directory);

int f_closedir(struct dirent* directory);

struct dirent* f_mkdir(char* path_name);

int f_rmdir(char* path_name,int flag);

int f_changeMod(int inode, int permission);

int f_userAuthen(char* username, char* password);

#endif