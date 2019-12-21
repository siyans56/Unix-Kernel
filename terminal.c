#include "terminal.h"
#include "sys_call.h"
#include "paging.h"
#include "filesystem.h"
#include "rtc.h"
#include "kb.h"
#include "types.h"
#include "lib.h"
#include "x86_desc.h"

/*
init_terminals:
functionality: Initializes terminal struct values
input:  None
outpu: None
Effects: Sets correct initial vakues to all member variables
*/
void init_terminals() {
  int i;
  curr_terminal = 0; // global variable to track the visible terminal
  running_terminal = 0; // global vairable to track the executing terminal
  char t_kb_buf[KB_BUF_SIZE] = { NULL }; // main kb buffer
  char t_kb_prev_buf[3][KB_BUF_SIZE] = { {NULL}, {NULL}, {NULL} };
  int t_kb_prev_buf_index[3] = { KB_EMPTY, KB_EMPTY, KB_EMPTY };

  int o; // I used o because I hate myself
  for (o = 0; o < NUM_PROCESSES; o++) {
    current_processses_running[o] = FREE; // set all current processes to free
  }

  // set all the data per terminal
  for (i = 0; i < NUM_TERMS; i++) {
    memcpy(terminal[i].kb_buf, t_kb_buf, KB_BUF_SIZE);
    memcpy(terminal[i].kb_prev_buf, t_kb_prev_buf, KB_BUF_SIZE*3);
    memcpy(terminal[i].kb_prev_buf_index, t_kb_prev_buf_index, 32*3);
    terminal[i].kb_buf_index = KB_EMPTY;
    terminal[i].current_prev = CLEAR;

    terminal[i].t_screen_x = CLEAR; // x position for terminal
    terminal[i].t_screen_y = CLEAR; // y position for terminal
    terminal[i].visited = CLEAR; // first time visit flag
    terminal[i].total_processes = CLEAR; // total running processes
    terminal[curr_terminal].read_flag = CLEAR; // keyboard read flag
    terminal[curr_terminal].curr_pid = -1; // current pid

    terminal[i].vid_mem = (char *)(VIDEO + (_4KB * (i+1)));

  }
  terminal[0].visited = VISTED; // automatically visit first terminal window

  clear_vmems(); // set video memory pages to initial state

}

/*
switch_terminals
functionality: Properly switches between two terminals on screen
input:  None
outpu: None
Effects: Loads and saves to video memory pages
*/
void switch_terminals(int new) {

    if (new == curr_terminal) return; // do not switch if aimed at same terminal

    // SAVE: current terminal vmem, cursor, input buffer
    save_term(curr_terminal, new);

    // SAVE: kb buffer, indices, prev inputs
    save_kb(curr_terminal, new);

    // SWITCH TO: new temrinal vmem, cursor, input buffer
    curr_terminal = new;

    // LOAD: current terminal vmem, cursor, input buffer
    load_term(curr_terminal);

    // LOAD: kb buffer, indices, prev inputs
    load_kb(curr_terminal);

}
