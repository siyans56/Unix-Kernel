#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "types.h"
#include "lib.h"

// various filesystem macros
#define SIZE_OF_BLOCKS 4096
#define STATS_SIZE 64
#define MAX_NUM_DENTRY 63
#define MAX_ENTRY_LEN 32
#define NUM_DATA_BLOCKS ((SIZE_OF_BLOCKS / 4) - 1)
#define DENTRY_RES_LEN 24
#define FSTATS_RES_LEN 52
#define FOUR_B_OFFSET 4

// for read, write, close, open ret_vals
#define FS_SUCCESS 0
#define FS_FAIL -1

// filetypes
#define RTC_TYPE 0
#define FOLDER_TYPE 1
#define FILE_TYPE 2


typedef struct {
    int8_t filename[MAX_ENTRY_LEN]; //name of the file
    uint32_t filetype; //type of the file(0, 1, 2)
    uint32_t inode_index; //index of the inode
    int8_t reserved[DENTRY_RES_LEN]; //reserved memory
} dentry_t;

typedef struct {
    uint32_t total_dirs; // # of directories
    uint32_t total_inodes; // # of inodes
    uint32_t total_data; // # of data blocks
    uint8_t reserved[FSTATS_RES_LEN]; //reserved memory
} fstats_t;

typedef struct {
    uint32_t length;
    uint32_t node_data[NUM_DATA_BLOCKS];
} inode_t;

typedef struct {
    uint32_t data[NUM_DATA_BLOCKS + 1];
} block_data_t;


//reads a given dentry by the file name
extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
//reads a given dentry by the index of the dentry
extern int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
//read the data
extern int32_t read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);
//initialize all globals in filesystem
extern void init_files(uint32_t fs_start);

//read the file
extern int32_t file_read(int32_t fd, void *buf, int32_t nbytes);
//write to file
extern int32_t file_write(int32_t fd, const void *buf, int32_t nbytes);
//close file
extern int32_t file_close(int32_t fd);
//open file
extern int32_t file_open(const uint8_t* filename);

extern int32_t dir_read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes);
//close directory
extern int32_t dir_close(int32_t fd);
//open directory
extern int32_t dir_open(const uint8_t* filename);

//testing functions
extern uint32_t check_invalid_block(uint32_t block_loc, uint32_t inode);
extern int32_t check_fs_init();
//get the length of the file from the inode
extern int32_t get_file_size(uint32_t node_index);

#endif
