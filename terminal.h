#include "filesystem.h"
#include "paging.h"

#define KB_BUF_SIZE 128 // size of kb_buf
#define VIDEO       0xB8000
#define KB_EMPTY 7
#define NUM_PROCESSES 6
#define NUM_TERMS 3
#define _4KB 4096
#define VISTED 1
#define CLEAR 0

int curr_terminal; // global for visible terminal
int running_terminal; // global for running terminal

typedef struct {
  // kb buf array
  char kb_buf[KB_BUF_SIZE]; // main keyboard buffer for user input
  char kb_prev_buf[3][KB_BUF_SIZE]; // buffer to hold the last command
  int kb_buf_index;//EMPTY; // current index to write to in kb_buf
  int kb_prev_buf_index[3];
  int current_prev;

  int t_screen_x;
  int t_screen_y;

  char* vid_mem;
  int total_processes;
  int visited;
  int read_flag;
  int curr_pid;
  // int32_t esp;
  // int32_t ebp;

} term_t;

term_t terminal[3]; // global array of our terminal window structs


extern void init_terminals();

extern void switch_terminals(int terminal);
