#include "filesystem.h"
#include "paging.h"

// constants used
#define MAX_FILE_OPS 8
#define JUMP_TABLE 4
#define NUM_PROCESSES 6
#define SYS_CALL_VEC 0x80
#define FREE 0
#define IN_USE 1
#define WRITE_INDEX 3
#define READ_INDEX 2
#define SIX_FOPS_BEGIN 2
#define PCB_MASK 0xFFFFE000
#define BIG_NUMBER 100000
#define START_ADDRESS 0x800087
#define _128MB 32
#define _4MB 0x400000
#define _136MB 0x8800000
#define _132MB 0x8400000
#define __128MB 0x8000000
#define EIGHT_MB 0x800000
#define STACK_SIZE 0x2000
#define VIRTUAL_ADDR 0x8048000
#define STACK_START 0x7FE000
#define BYTE_ZERO 0x7f
#define BYTE_ONE 0x45
#define BYTE_TWO 0x4c
#define BYTE_THREE 0x46
#define PAGE 32
#define AMOUNT_OF_BYTES 4
#define TSS_OFFSET 4
#define SYS_BUF_SIZE 4
#define READ_SIZE 24
#define FAIL -1
#define GOOD 0
#define IF_FLAG 0x200
#define ESP_USER 0x83FFFFC
#define RTC_INODE -2
#define PHYS_ADDR 0xB8000
#define MAX_BYTES 1025


uint8_t current_processses_running[6];

typedef struct {
     int32_t (*open)(const uint8_t* filename); // open function pointer
     int32_t (*close)(int32_t fd); // close function pointer
     int32_t (*read)(int32_t fd, void * buf, int32_t nbytes); // read function pointer
     int32_t (*write)(int32_t fd, const void *buf, int32_t nbytes); // wrote function pointer
} fops;

typedef struct {
    fops file_jumptable; // array jumptable for open, read, write, and close
    int32_t file_inode; //index of the inode
    uint32_t file_pos; //position of file
    uint32_t file_flags; //flags of file
} file_desc_t;

typedef struct {
    file_desc_t fd_arr[MAX_FILE_OPS]; // array holding the 8 file descriptors
    uint32_t curr_pid; // holds current process identifier
    uint32_t parent_pid; // holds parent process identifier
    uint32_t parent_base_pointer; // stores base pointer
    uint32_t parent_stack_pointer; // stores stack pointer
    uint32_t stack_pointer;
    uint32_t base_pointer;
    uint32_t start_address;
    char args_buf[1025];
    int is_base;
} pcb_t;

// halt the current process
extern int32_t halt(uint8_t status);
// execute a given command
extern int32_t execute(const uint8_t * command);
// generic read system call
extern int32_t read(int32_t fd, void * buf, int32_t nbytes);
// generic write system call
extern int32_t write(int32_t fd, const void * buf, int32_t nybtes);
// open any file to fd array
extern int32_t open(const uint8_t * filename);
// close an open file from fd array
extern int32_t close(int32_t fd);
// placeholder if no file operation func exists
extern int32_t no_fops_func();
// get the current pcb struct pointer
extern pcb_t * curr_pcb(void);
// get the parent pcb struct
extern pcb_t * get_parent_pcb(uint32_t parent_pid);
// maps the texs mode video memory into user space
extern int32_t vidmap (uint8_t** screen_start);
// gets args from shell
extern int32_t getargs(uint8_t* buf, int32_t nbytes);
// extra credit - not implemented, just a placeholder
extern int32_t set_handler(int32_t signum, void * handler_address);
// extra credit - not implemented, just a placeholder
extern int32_t sigreturn(void);
