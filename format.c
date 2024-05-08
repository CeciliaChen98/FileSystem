#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "filesystem.h"

#define DEFAULT_SIZE_MB 1
#define MB_TO_BYTES(mb) ((mb) * 1024 * 1024)
#define BLOCK_SIZE 512
#define N_DBLOCKS 10
#define N_IBLOCKS 4
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct inode))
#define FILETYPE 0
#define DIRECTORYTYPE 1
#define SUPERUSER 0

void create_disk_image(const char* file_name, int size_mb) {
    FILE* file = fopen(file_name, "wb");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Initialize superblock
    struct Superblock sb = {BLOCK_SIZE,0, 8, 6, 11};  
    fwrite(&sb, sizeof(sb), 1, file);

    // Initialize inode-1 to store the user info
    struct inode user_inode = {-1, FILETYPE, RWx, -1, 1, SUPERUSER, sizeof(struct User), time(NULL), {0}, {0}, 0};
    if (fwrite(&user_inode, sizeof(user_inode), 1, file) != 1) {
        printf("Error when writing userinode.\n");
    }

    if (fseek(file, (sb.inode_offset + 1)*BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Initialize the root directory inode
    struct inode root_inode = {0,DIRECTORYTYPE, RWx, -1, 1, SUPERUSER, 4 * sizeof(struct dirent), time(NULL), {1}, {0}, 0};
    if (fwrite(&root_inode, sizeof(root_inode), 1, file) != 1) {
        printf("Error when writing rootinode.\n");
    }

    // Initialize a file inode with fake content
    char file_content[] = "Hello, this is some text in the file.\n";
    int file_size = sizeof(file_content);
    struct inode file_inode = {1,FILETYPE, RWx, -1, 1, SUPERUSER, file_size, time(NULL), {2}, {0}, 0};
    if (fwrite(&file_inode, sizeof(file_inode), 1, file) != 1) {
        printf("Error when writing fileinode.\n");
    }

    struct inode layerone_inode = {2,DIRECTORYTYPE, NONE, 0, 1, SUPERUSER, 3*sizeof(struct dirent), time(NULL), {3}, {0}, 0};
    if (fwrite(&layerone_inode, sizeof(layerone_inode), 1, file) != 1) {
        printf("Error when writing layeroneinode.\n");
    }

    struct inode layertwo_inode = {3,DIRECTORYTYPE, NONE, 2, 1, SUPERUSER, 4*sizeof(struct dirent), time(NULL), {4}, {0}, 0};
    if (fwrite(&layertwo_inode, sizeof(layertwo_inode), 1, file) != 1) {
        printf("Error when writing layertwoinode.\n");
    }

    char path[] = "layerone/layertwo/more.txt";
    int path_size = sizeof(path);
    struct inode layertwo_path_inode = {4,FILETYPE, RWx, 3, 1, SUPERUSER, path_size, time(NULL), {5}, {0}, 0};
    if (fwrite(&layertwo_path_inode, sizeof(layertwo_path_inode), 1, file) != 1) {
        printf("Error when writing layertwo_path_inode.\n");
    }

    char more[] = "Once upon a time in a quiet village beside a whispering river, there lived an old baker known \
    for his extraordinary pastries. Every morning, the baker would wake up before dawn to prepare his dough \
    and mix spices brought from distant lands. One peculiar morning, a mischievous wind carried away his \
    recipe book through the open window. Determined not to disappoint his loyal customers, the baker decided \
    to recreate his recipes from memory. With a sprinkle of creativity and a dash of hope, he concocted a \
    new pastry. This delightful creation was unlike any other and brought more joy and curious visitors to \
    the village than ever before. The baker not only found his lost recipes later that day tucked away in \
    his garden by the playful wind, but he also discovered the true magic ingredient: a pinch of unexpected adventure.\
    Once upon a time in a quiet village beside a whispering river, there lived an old baker known \
    for his extraordinary pastries. Every morning, the baker would wake up before dawn to prepare his dough \
    and mix spices brought from distant lands. One peculiar morning, a mischievous wind carried away his \
    recipe book through the open window. Determined not to disappoint his loyal customers, the baker decided \
    to recreate his recipes from memory. With a sprinkle of creativity and a dash of hope, he concocted a \
    new pastry. This delightful creation was unlike any other and brought more joy and curious visitors to \
    the village than ever before. The baker not only found his lost recipes later that day tucked away in \
    his garden by the playful wind, but he also discovered the true magic ingredient: a pinch of unexpected adventure.\
    Once upon a time in a quiet village beside a whispering river, there lived an old baker known \
    for his extraordinary pastries. Every morning, the baker would wake up before dawn to prepare his dough \
    and mix spices brought from distant lands. One peculiar morning, a mischievous wind carried away his \
    recipe book through the open window. Determined not to disappoint his loyal customers, the baker decided \
    to recreate his recipes from memory. With a sprinkle of creativity and a dash of hope, he concocted a \
    new pastry. This delightful creation was unlike any other and brought more joy and curious visitors to \
    the village than ever before.\n";

    int more_size = sizeof(more);
    printf("more size: %d\n", more_size);
    struct inode layertwo_more_inode = {5,FILETYPE, RWx, 3, 1, SUPERUSER, more_size, time(NULL), {6, 7,8,9,10}, {0}, 0};
    if (fwrite(&layertwo_more_inode, sizeof(layertwo_more_inode), 1, file) != 1) {
        printf("Error when writing layertwo_more_inode.\n");
    }


    // Initialize remaining inodes and link them as free inodes
    struct inode empty_inode = {0, 0, 0, -1, 0, 0, 0, 0, {0}, {0}, 0};
    for (int i = 6; i < 8 * INODES_PER_BLOCK; ++i) {
        empty_inode.index = i;
        if (i < 8 * INODES_PER_BLOCK - 1) {
            empty_inode.parent = i + 1;
        } else {
            empty_inode.parent = -1;
        }
        if (fwrite(&empty_inode, sizeof(struct inode), 1, file) != 1) {
            printf("Error when writing free inode.\n");
        }
    }


    int data_block_offset = (sb.data_offset + 1) * BLOCK_SIZE;
    if (fseek(file, data_block_offset, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    struct User superUser = {
        .username = "chen",
        .uid = 0,
        .password = "09080223"
    };

    struct User normalUser = {
        .username = "dianna",
        .uid = 1,
        .password = "12345678"
    };
    fwrite(&superUser, sizeof(struct User), 1, file);
    fwrite(&normalUser, sizeof(struct User), 1, file);

    if (fseek(file, data_block_offset + BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    // Write root directory data block with directory entries
    struct dirent root_entries[4] = {{0, 1, -1, "."}, {0, 1, -1, ".."}, {1, 0, -1, "file.txt"},{2, 1, -1, "layerone"}};
    fwrite(root_entries, sizeof(root_entries), 1, file);

    // Write file content block
    char block[BLOCK_SIZE] = {0};
    memcpy(block, file_content, file_size);
    if (fseek(file, data_block_offset + 2*BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fwrite(block, BLOCK_SIZE, 1, file);

    // Write layerone directory data block with directory entries
    if (fseek(file, data_block_offset + 3*BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    struct dirent layerone_entries[3] = {{2, DIRECTORYTYPE, -1, "."}, {0, DIRECTORYTYPE, -1, ".."}, {3, DIRECTORYTYPE, -1, "layertwo"}};
    fwrite(layerone_entries, sizeof(layerone_entries), 1, file);

    // Write layertwo directory data block with directory entries
    if (fseek(file, data_block_offset + 4*BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    struct dirent layertwo_entries[4] = {{3, DIRECTORYTYPE, -1, "."}, {2, DIRECTORYTYPE, -1, ".."}, {5, FILETYPE, -1, "more.txt"}, {4, FILETYPE, -1, "path.txt"}};
    fwrite(layertwo_entries, sizeof(layertwo_entries), 1, file);

    // Write layertwo path file content block
    //write path.txt 
    if (fseek(file, data_block_offset + 5*BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    memcpy(block, path, path_size);
    fwrite(block, BLOCK_SIZE, 1, file);

    // Write layertwo more file content block
    if (fseek(file, data_block_offset + 6*BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fwrite(more, more_size, 1, file);

    // Initialize free data blocks
    if (fseek(file, data_block_offset + sb.free_block* BLOCK_SIZE, SEEK_SET) != 0) {
        perror("Error positioning file pointer to data region");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    int total_blocks = size_mb * 1024 * 1024 / BLOCK_SIZE;
    int free_blocks = total_blocks - (sb.data_offset + sb.free_block);  // Adjust for root and file block
    for (int i = 0; i < free_blocks; ++i) {
        int next_free_block;
        if (i < free_blocks - 1) {
            next_free_block = i + sb.free_block + 1;
        } else {
            next_free_block = -1;
        }
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
    printf("User inode:\n");
    if (fread(&in, sizeof(struct inode), 1, file) != 1) {
            printf(" reading user inode failed.\n");
    }
    printf("Inode %d: Type %d, Parent:%d, uid: %d, Permissions %d, Size %d, Mtime %d\n",
               in.index, in.type, in.parent, in.uid, in.permissions, in.size, in.mtime);

    printf("\nInodes:\n");
    fseek(file, (sb.inode_offset+1) * BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < 8 * INODES_PER_BLOCK; ++i) {
        if (fread(&in, sizeof(struct inode), 1, file) != 1 || in.mtime == 0) {
            printf("Finish.\n");
            break;  // Stop if we fail to read an inode
        }
        printf("Inode %d: Type %d, Parent:%d, uid: %d, Permissions %d, Size %d, Mtime %d\n",
               in.index, in.type, in.parent, in.uid, in.permissions, in.size, in.mtime);
    }

    // Assuming directory entries are located right at the data offset
    printf("\nDirectory Entries:\n");
    fseek(file, (sb.data_offset+1) * BLOCK_SIZE, SEEK_SET);
    struct dirent de;
    struct User user;
    if (fread(&user, sizeof(struct User), 1, file) == 1) {
        printf("User: username: %s, userid: %d, password: %s\n", user.username, user.uid, user.password);
    }

    if (fread(&user, sizeof(struct User), 1, file) == 1) {
        printf("User: username: %s, userid: %d, password: %s\n", user.username, user.uid, user.password);
    }

    fseek(file, (sb.data_offset+2) * BLOCK_SIZE, SEEK_SET);

    while (fread(&de, sizeof(struct dirent), 1, file) == 1) {
        if (strlen(de.name) > 0) {  // Only print valid entries
            printf("  Entry: %s, Inode: %d, Type: %d\n", de.name, de.inode, de.type);
        }
    }

    // print freeblocks
    printf("\nFree Blocks:\n");
    int start = (sb.data_offset + sb.free_block + 1) * BLOCK_SIZE;
    fseek(file, start, SEEK_SET);
    int free_block;
    int count =0;
    while (fread(&free_block, sizeof(int), 1, file) == 1 && count < 10) {
         printf("  Free Block: block %d, next free block is %d\n", count + sb.free_block, free_block);
         start = start + BLOCK_SIZE;
         fseek(file, start, SEEK_SET);
         count ++;
    }

    printf("Test the more file\n");
    fseek(file, (sb.data_offset + 8) * BLOCK_SIZE, SEEK_SET);
    char more[3000];
    fread(more, 3000, 1, file);
    printf("more: %s\n", more);


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
    print_disk_contents(file_name);
    return 0;
}

