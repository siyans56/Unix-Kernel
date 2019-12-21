#include "types.h"

// Constants:
#define KB_ON 1 // IRQ number for keyboard
#define DATA_PORT 0x60 // data port for inb from keyboard
#define COMMAND_PORT 0x64 // command port for inb from keyboard
#define NUM_CODES 59 // number of scancodes needed in char map
#define NUM_MODES 4 // number of modes for keyboard
#define KB_BUF_SIZE 128 // size of kb_buf
#define SPECIAL_KEY 2 // key I use to handle alterring keypresses
#define BUF_LAST 127 // last valid element in buffer
#define NEWLINE 10 // \n newline char
#define EMPTY 0 // empty
#define BACKSPACE 0x0E // scancode value
#define LEFT_SHIFT 0x2A // scancode value
#define RIGHT_SHIFT 0x36 // scancode value
#define CAPS_PRESSED 0x3A // scancode value
#define CONTROL 0x1D // scancode value
#define ALT 56
#define ALT_RELEASE 184
#define LEFT_SHIFT_RELEASE 0xAA // scancode value
#define RIGHT_SHIFT_RELEASE 0xB6 // scancode value
#define CONTROL_RELEASE 0x9D // scancode value
#define PASS 0 // pass test
#define FAIL -1 // fail test
#define CLEAR 0 // clear value
#define SET 1 // set value
#define CAPSLOCK 2 // capslock mode value
#define UP_ARROW 72 // up arrow (cursor)
#define F1 59
#define F2 60
#define F3 61

// Global variables:
extern int kb_buf_index; // main keyboard buffer index
extern char kb_prev_buf[3][128]; // buffer used for clear screen call
extern int kb_prev_buf_index[3]; // index of the previous_command


// Functions:

// Initializes keyboard
extern void init_kb();

 // main funciton to read input
extern void read_kb();

// sets keyboard mode
extern void set_kb_mode(uint16_t scancode);

// read system call for keyboard
extern int32_t kb_read_syscall(int32_t fd, void *buf, int32_t nbytes);

// write system call for keyboard
extern int32_t kb_write_syscall(int32_t fd, const void* buf, int32_t nbytes);

// open system call for keyboard
extern int32_t kb_open_syscall(const uint8_t* filename);

// close system call for keyboard
extern int32_t kb_close_syscall(int32_t fd);

// print out character
extern void kb_print(char to_print);

// clear keyboard buffer
extern void clear_kb_buf();

// handle backpsace key input
extern void backspace();

// clear screen aside from current input buffer
extern void special_clear_screen();

// restores last used commadn (arrow up)
extern void previous_command();

// restores a more recent used command (arrow down)
extern void recent_command();

extern void choose_terminals(uint16_t response);

extern void save_kb(int current_terminal, int new);
extern void load_kb(int current_terminal);
