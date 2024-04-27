#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

#define DEFAULT_SIZE_MB 1
#define MB_TO_BYTES(mb) ((mb) * 1024 * 1024)
#define BLOCK_SIZE 512
#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct inode))

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

struct Superblock {
    int size;
    int inode_offset;
    int data_offset;
    int free_inode;    // Index of first free inode
    int free_block;
    char padding[BLOCK_SIZE - 5*sizeof(int)];
};

struct inode {
    int index;
    int type;
    int permissions;
    int parent;        // Used to store the next free inode
    int nlink;
    int size;
    int mtime;
    int dblocks[N_DBLOCKS];
    int iblocks[N_IBLOCKS];
    int i2block;
};

struct dirent {
    int inode;
    int type;
    int offset;
    char name[116];
};

void create_disk_image(const char* file_name, int size_mb) {
    FILE* file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Initialize superblock
    struct Superblock sb = {BLOCK_SIZE,0, 8, 2, 2};  // Free inode starts at index 2
    fwrite(&sb, sizeof(sb), 1, file);

    // Initialize the root directory inode
    struct inode root_inode = {0,1, 0, -1, 1, 3 * sizeof(struct dirent), time(NULL), {0}, {0}, 0};
    fwrite(&root_inode, sizeof(root_inode), 1, file);

    // Initialize a file inode with fake content
    char file_content[] = "Hello, this is some text in the file.";
    int file_size = sizeof(file_content);
    struct inode file_inode = {1,0, RWx, -1, 1, file_size, time(NULL), {1, 0}, {0}, 0};
    fwrite(&file_inode, sizeof(file_inode), 1, file);

    // Initialize remaining inodes and link them as free inodes
    struct inode empty_inode = {0, 0, 0, -1, 0, 0, 0, {0}, {0}, 0};
    for (int i = 2; i < 8 * INODES_PER_BLOCK; ++i) {
        empty_inode.index = i;
        empty_inode.parent = (i < (8 * INODES_PER_BLOCK - 1)) ? i + 1 : -1;
        fwrite(&empty_inode, sizeof(empty_inode), 1, file);
    }

    int data_block_offset = (sb.data_offset + 1) * BLOCK_SIZE;
    if (fseek(file, data_block_offset, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Write root directory data block with directory entries
    struct dirent root_entries[3] = {{0, 1, -1, "."}, {-1, -1, -1, ".."}, {1, 0, -1, "file.txt"}};
    fwrite(root_entries, sizeof(root_entries), 1, file);

    // Write file content block
    char block[BLOCK_SIZE] = {0};
    memcpy(block, file_content, file_size);
    if (fseek(file, data_block_offset + BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fwrite(block, BLOCK_SIZE, 1, file);

    // Initialize free data blocks
    int total_blocks = size_mb * 1024 * 1024 / BLOCK_SIZE;
    int free_blocks = total_blocks - (sb.data_offset + 3);  // Adjust for root and file block
    for (int i = 0; i < free_blocks; ++i) {
        int next_free_block = (i < free_blocks - 1) ? 3+i : -1;
        memset(block, 0, BLOCK_SIZE);  // Clear the block
        memcpy(block, &next_free_block, sizeof(int));
        fwrite(block, BLOCK_SIZE, 1, file);
    }

    fclose(file);
    printf("Disk image '%s' created with size %dMB, %d free blocks initialized.\n", file_name, size_mb, free_blocks);
}

void print_disk_contents(const char* file_name) {
    FILE *file = fopen(file_name, "rb");
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
        printf("Inode %d: Type %d, Permissions %d, Size %d, Mtime %d\n",
               i, in.type, in.permissions, in.size, in.mtime);
    }

    // Assuming directory entries are located right at the data offset
    printf("\nDirectory Entries:\n");
    fseek(file, (sb.data_offset+1) * BLOCK_SIZE, SEEK_SET);
    struct dirent de;
    while (fread(&de, sizeof(struct dirent), 1, file) == 1) {
        if (strlen(de.name) > 0) {  // Only print valid entries
            printf("  Entry: %s, Inode: %d, Type: %d\n", de.name, de.inode, de.type);
        }
    }

    fclose(file);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [-s size_mb]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int size_mb = DEFAULT_SIZE_MB;
    char* file_name = argv[1];
    int option;
    while ((option = getopt(argc, argv, "s:")) != -1) {
        if (option == 's') {
            size_mb = atoi(optarg);
        } else {
            fprintf(stderr, "Usage: %s <filename> [-s size_mb]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    create_disk_image(file_name, size_mb);
    //print_disk_contents(file_name);
    return 0;
}

