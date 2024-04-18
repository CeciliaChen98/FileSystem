#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_SIZE_MB 1
#define MB_TO_BYTES(mb) ((mb) * 1024 * 1024)
#define BLOCK_SIZE 512
#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct inode))

enum Permission {
    NONE = 0,
    READ_ONLY = 1,
    WRITE_ONLY = 2,
    READ_WRITE = 3
};

struct Superblock {
    int inode_offset;
    int data_offset;
    int free_inode;    // Index of first free inode
    int free_block;
};

struct inode {
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
    char name[255];
};

void create_disk_image(const char* file_name, int size_mb) {
    FILE* file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Initialize superblock
    struct Superblock sb = {1, 1 + 8, 2, 1 + 8 + 2};  // Free inode starts at index 2
    fwrite(&sb, sizeof(sb), 1, file);

    // Initialize the root directory inode
    struct inode root_inode = {1, 0, -1, 1, 2 * sizeof(struct dirent), time(NULL), {sb.data_offset, 0}, {0}, 0};
    fwrite(&root_inode, sizeof(root_inode), 1, file);

    // Initialize a file inode with fake content
    char file_content[] = "Hello, this is some text in the file.";
    int file_size = sizeof(file_content);
    struct inode file_inode = {0, READ_WRITE, -1, 1, file_size, time(NULL), {sb.data_offset + 1, 0}, {0}, 0};
    fwrite(&file_inode, sizeof(file_inode), 1, file);

    // Initialize remaining inodes and link them as free inodes
    struct inode empty_inode = {0, 0, -1, 0, 0, 0, {0}, {0}, 0};
    for (int i = 2; i < 8 * INODES_PER_BLOCK; ++i) {
        empty_inode.parent = (i < (8 * INODES_PER_BLOCK - 1)) ? i + 1 : -1;
        fwrite(&empty_inode, sizeof(empty_inode), 1, file);
    }

    // Write root directory data block with directory entries
    struct dirent root_entries[3] = {{0, 1, 0, "."}, {0, 1, 1, ".."}, {1, 0, 2, "file.txt"}};
    fwrite(root_entries, sizeof(root_entries), 1, file);

    // Write file content block
    char block[BLOCK_SIZE] = {0};
    memcpy(block, file_content, file_size);
    fwrite(block, BLOCK_SIZE, 1, file);

    // Initialize free data blocks
    int total_blocks = size_mb * 1024 * 1024 / BLOCK_SIZE;
    int free_blocks = total_blocks - (sb.data_offset + 2);  // Adjust for root and file block
    for (int i = 0; i < free_blocks; ++i) {
        int next_free_block = (i < free_blocks - 1) ? sb.data_offset + 2 + i : -1;
        memcpy(block, &next_free_block, sizeof(int));
        fwrite(block, BLOCK_SIZE, 1, file);
    }

    fclose(file);
    printf("Disk image '%s' created with size %dMB, %d free blocks initialized.\n", file_name, size_mb, free_blocks);
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
    return 0;
}
