#include "kb.h"
#include "i8259.h"
#include "lib.h"
#include "types.h"
#include "terminal.h"

// array of characters that maps scancode to proper 0-9, a-z ASCII characters
char keys[NUM_MODES][NUM_CODES] = {
	// Mode 0: No alteration
	{'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
   '-', '=', SPECIAL_KEY, '\0',	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
   'o', 'p', '[', ']', '\n', SPECIAL_KEY, 'a', 's',	 'd', 'f', 'g', 'h',
    'j', 'k', 'l' , ';', '\'', '`', SPECIAL_KEY, '\\', 'z', 'x', 'c', 'v',
	 'b', 'n', 'm',',', '.', '/', SPECIAL_KEY, '\0', '\0', ' ', SPECIAL_KEY},
	// Mode 1: Shift only
	{'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
   '_', '+', SPECIAL_KEY, '\0',	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{', '}', '\n', SPECIAL_KEY, 'A', 'S',	 'D', 'F', 'G', 'H',
     'J', 'K', 'L' , ':', '"', '~', SPECIAL_KEY, '|', 'Z', 'X', 'C', 'V',
	 'B', 'N', 'M', '<', '>', '?', SPECIAL_KEY, '\0', '\0', ' ', SPECIAL_KEY},
	// Mode 2: Capslock only
	{'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
   '-', '=', SPECIAL_KEY, '\0',	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '[', ']', '\n', SPECIAL_KEY, 'A', 'S',	 'D', 'F', 'G', 'H',
     'J', 'K', 'L' , ';', '\'', '`', SPECIAL_KEY, '\\', 'Z', 'X', 'C', 'V',
	 'B', 'N', 'M', ',', '.', '/', SPECIAL_KEY, '\0', '\0', ' ', SPECIAL_KEY},
	// Mode 3: Capslock and shift
	{'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
   '_', '+', SPECIAL_KEY, '\0', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
    'o', 'p', '{', '}', '\n', SPECIAL_KEY, 'a', 's',	 'd', 'f', 'g', 'h',
     'j', 'k', 'l' , ':', '"', '~', SPECIAL_KEY, '\\', 'z', 'x', 'c', 'v',
	 'b', 'n', 'm', '<', '>', '?', SPECIAL_KEY, '\0', '\0', ' ', SPECIAL_KEY}
};

// int read_flag = 0;

// Global variables:
char kb_buf[KB_BUF_SIZE] = { NULL }; // main keyboard buffer for user input
char kb_prev_buf[3][KB_BUF_SIZE]  = { {NULL}, {NULL}, {NULL} }; // buffer to hold the last command
int kb_buf_index = 7;//EMPTY; // current index to write to in kb_buf
int kb_prev_buf_index[3] = {7, 7, 7};
int current_mode = CLEAR; // set to default no shift, no cap
int control_flag = CLEAR; // check if control was pressed
int alt_flag = CLEAR;
int current_prev = 0;

/* init_kb
* Inputs: None
* Outputs: None
* Side Effects: enables keyboard interrupts
*/
void init_kb() {
  // enable keyboard interrupts for keyboard, KB_ON == 1
  enable_irq(KB_ON);

}

/* read_kb
* Inputs: None
* Outputs: None
* Side Effects: prints the key that was pressed to the screen
*/
void read_kb() {

  // character holding proper ascii value from scancode
  unsigned char res;
	// printf("got hfere\n" );
  // begin critical section
  cli();

  // status scancode
  // uint16_t status = inb(COMMAND_PORT);

  // response scan code
  uint16_t response = inb(DATA_PORT);

	// printf("%d\n", response);

	if (response == 72) previous_command();
	if (response == 80) recent_command();

  // continue to loop until the status buffer is empty
  // while ((status & 1) != CLEAR) {

  // check for any alterring keypresses (Shift, Control, Capslock, Alt etc.)
  set_kb_mode(response);

	/* alt: 56, f1: 59, f2: 60, f3: 61
	*/
	if (response >= F1 && response <= F3) choose_terminals(response);

  // printing ascii character to screen, but only for key press, NOT key release
  if(response < NUM_CODES) {
    res = keys[current_mode][response]; // grab ASCII char from map
    switch (res) {
      case '\0': // if invalid scancode, do nothing
      case SPECIAL_KEY: // if valid scancode but alterring key, do nothing
        break;
      case 'l':
      case 'L': //if uppercase or lowercase L
        if (control_flag) { // if control flag is set
					special_clear_screen(); // special clear func for CTRL+L
					break;
        }
      default:
				kb_print(res); // print the valid key to screen
				break;
    }
  }

    // getting next response and status values
    // response = inb(DATA_PORT);
    // status = inb(COMMAND_PORT);
  // }

  // sending end of interrupt
  send_eoi(KB_ON);

  // end of critical section
  sti();
}

/* kb_print
* Inputs: character to be printed
* Outputs: None
* Side Effects: Displays character to screen
*/
void kb_print(char to_print) {
	if (kb_buf_index == BUF_LAST) { // if buffer is full
		if (to_print == NEWLINE) { // only accept newline if full
			terminal[curr_terminal].read_flag = 1;
      kb_buf[kb_buf_index] = to_print; // store into kb buffer
			putc(to_print); // print to screen
      // // read/write syscall test
			// kb_read_syscall(0, kb_test_buf, KB_BUF_SIZE);
			// kb_write_syscall(0, kb_test_buf, KB_BUF_SIZE);
			// clear_kb_buf();
			return;
		}
		else { // if any other key, do nothing until buffer is emptied
			return;
		}
	}
	else if (to_print == NEWLINE) { // if buffer is not empty but we get newline
		terminal[curr_terminal].read_flag = 1;
		// kb_buf[kb_buf_index] = to_print; // store into kb buffer
		// kb_buf_index++; // increment index
    putc(to_print); // print to screen
    // // read/write syscall test
		// kb_read_syscall(0, kb_test_buf, kb_buf_index);
		// kb_write_syscall(0, kb_test_buf, kb_buf_index);
		// clear_kb_buf(); // clear buffer
		return;
	}
	else { // normal key, save and print
    kb_buf[kb_buf_index] = to_print; // store into kb buffer
		kb_buf_index++; // increment index
    putc(to_print); // print to screen
	}
}

/* clear_kb_buf
* Inputs: None
* Outputs: None
* Side Effects: Clears the keyboard buffer and resets index
*/
void clear_kb_buf() {
	int i;
	for (i = 0; i < KB_BUF_SIZE; i++) { // for each buffer element, set to NULL
		kb_buf[i] = NULL;
	}
	kb_buf_index = 7;//EMPTY; // reset index
}

void special_clear_screen() {
	int i = 7; // want to start at "start" of kb buf
	clear_screen(); // call given function

	printf("391OS> "); // reprint shell prompt

	// print buffer to the top of screen
	while (i < 128 && kb_buf[i] != NULL) {
		putc(kb_buf[i]);
		i++;
	}
}

/* set_kb_mode
* Inputs: 16-bit scancode from PS/2 keyboard response register
* Outputs: None
* Side Effects: Handles alterring keypresses before kb_print is called
*/
void set_kb_mode(uint16_t scancode) {
    switch (scancode) {
      case BACKSPACE: // backpsace pressed
				backspace();
        break;
      case LEFT_SHIFT:
      case RIGHT_SHIFT: // Left or Right shifts
        current_mode++; // move mode up one
        break;
      case CAPS_PRESSED: // Capslock pressed
        if (current_mode > 1) current_mode-= CAPSLOCK; // if we had capslock already on, remove
        else current_mode+= CAPSLOCK; // otherwise set capslock
        break;
      case CONTROL: // Control pressed
        control_flag = SET; // set control flag
        break;
			case ALT:
				alt_flag = SET;
				break;
			case ALT_RELEASE:
				alt_flag = CLEAR;
				break;
      case LEFT_SHIFT_RELEASE:
      case RIGHT_SHIFT_RELEASE: // Left or Right Shifts released
        current_mode--; // move mode down one
        break;
      case CONTROL_RELEASE: // Control released
        control_flag = CLEAR; // clear control flag
        break;
    }
		if (current_mode < 0) current_mode = CLEAR;
}

/* backpsace
* Inputs: None
* Outputs: None
* Side Effects: Removes one character from kb_buf and clears a character from screen
*/
void backspace() {
	if (kb_buf_index > 7) {
	 kb_buf_index--; // if buffer not empty, subtract one char
	 kb_buf[kb_buf_index] = NULL;
	 clear_char(); // clear character from video memory
	}
}

void choose_terminals(uint16_t response) {
	send_eoi(KB_ON);

	if (!alt_flag) return;
	if (response == 59) switch_terminals(0);
	if (response == 60) switch_terminals(1);
	if (response == 61) switch_terminals(2);
}

void previous_command() {
	// (press(), (release) UP: 72 200, LEFT: 75, 203, DOWN: 80, 208, RIGHT: 77, 205
	int i = 7, j;
	// printf("REACHED\n");

	for (j = 0; j < KB_BUF_SIZE; j++) {
		backspace();
	}
	clear_kb_buf(); // clear whatever is currently in buffer to replace it with previous

	memcpy(kb_buf+7, kb_prev_buf[current_prev]+7, 121); // kb buf now has last command

	kb_buf_index = kb_prev_buf_index[current_prev]; // set the index;



	while (i < 128 && kb_buf[i] != NULL) { // display prev command to screen
		putc(kb_buf[i]);
		i++;
	}
	current_prev = (current_prev == 2) ? 2 : current_prev+1;
	// printf("%d", current_prev);
}

void recent_command() {
	// (press(), (release) UP: 72 200, LEFT: 75, 203, DOWN: 80, 208, RIGHT: 77, 205
	int i = 7, j;
	// printf("Curr prev: %d\n", current_prev);

	current_prev--;
	// printf("REACHED\n");

	for (j = 0; j < KB_BUF_SIZE; j++) {
		backspace();
	}
	clear_kb_buf(); // clear whatever is currently in buffer to replace it with previous

	if (current_prev == -1) {
		current_prev = 0;
		return;
	}

	memcpy(kb_buf+7, kb_prev_buf[current_prev]+7, 121); // kb buf now has last command

	kb_buf_index = kb_prev_buf_index[current_prev]; // set the index;



	while (i < 128 && kb_buf[i] != NULL) { // display prev command to screen
		putc(kb_buf[i]);
		i++;
	}
	// printf("%d", current_prev);

}

/* kb_read_syscall
* Inputs: void pointer to buffer, 32 bit value of bytes to read
* Outputs: 32 bit amount of bytes read
* Side Effects: Reads bytes from user buffer data to a specified buffer
*/
int32_t kb_read_syscall(int32_t fd, void * buf, int32_t nbytes) {
	// int z = 0;
	while (!terminal[curr_terminal].read_flag) { //keep checking until flag is clear/set)n
	}
	terminal[curr_terminal].read_flag = 0;
	// int i;
  int32_t size;
	if (nbytes < 0) return FAIL; // if bytes is invalid, error
	if (buf == NULL) return FAIL; // if buffer is invalid pointer, invalid

  size = (nbytes > KB_BUF_SIZE - 7) ? KB_BUF_SIZE : nbytes; // max size is 128 bytes

  memcpy(buf, kb_buf+7, size); // copy over to buf from kb_buf

	//"Shift" each saved command up by one
	memcpy(kb_prev_buf[2]+7, kb_prev_buf[1]+7, 121); //  save old second command as new third command
	kb_prev_buf_index[2] = kb_prev_buf_index[1]; // shift prev index
	memcpy(kb_prev_buf[1]+7, kb_prev_buf[0]+7, 121); //  save old first  command as new second command
	kb_prev_buf_index[1] = kb_prev_buf_index[0]; // shift prev index

	memcpy(kb_prev_buf[0]+7, kb_buf+7, 121); //  save command to first prev buf
	kb_prev_buf_index[0] = kb_buf_index; // save prev index

	// for (i = 7; i < 14; i++) {
	// 	putc(kb_prev_buf[0][i]);
	// }
	// putc('\n');
	//
	// 	for (i = 7; i < 14; i++) {
	// 		putc(kb_prev_buf[1][i]);
	// 	}
	// 	putc('\n');
	//
	// 		for (i = 7; i < 14; i++) {
	// 			putc(kb_prev_buf[2][i]);
	// 		}
	// 		putc('\n');

	clear_kb_buf();
	current_prev = 0; // reset prev command fflag
  return size; // return bytes copied over
}

/* kb_write_syscall
* Inputs:  void pointer to buffer to write, 32 bit value of bytes to read
* Outputs: None
* Side Effects: Writes bytes to screen from a given buffer
*/int32_t kb_write_syscall(int32_t fd, const void* buf, int32_t nbytes) {
  int32_t i, bytes = CLEAR;
  if (nbytes < 0) return FAIL; // if bytes is invalid, error
  if (buf == NULL) return FAIL; // if buffer is invalid pointer, invalid
	int8_t* ptr = (int8_t*)buf; // cast pointer to char pointer (8 bits)
	for (i = 0; i < nbytes; i++) { // print each character to screen
    putc(ptr[i]);
    bytes++; // count number of characters printed
  }
  return bytes; // return number of characters printed
}

/* kb_open_syscall
* Inputs: None
* Outputs: None
* Side Effects: Initializes terminal driver. Does nothing atm
*/
int32_t kb_open_syscall(const uint8_t* filename) {
  return PASS;
}

/* kb_close_syscall
* Inputs: None
* Outputs: None
* Side Effects: Closes terminal driver. Does nothing atm
*/
int32_t kb_close_syscall(int32_t fd) {
  return PASS;
}


/*
KB:
ports 0x0060-0x0064
Make read key, create an interrupt at interrupt 33, call it properly
*/
void save_kb(int current_terminal, int new) {
	memcpy(terminal[current_terminal].kb_buf, kb_buf, KB_BUF_SIZE);
	memcpy(terminal[current_terminal].kb_prev_buf, kb_prev_buf, KB_BUF_SIZE*3);
	memcpy(terminal[current_terminal].kb_prev_buf_index, kb_prev_buf_index, 4*3);
	terminal[current_terminal].kb_buf_index = kb_buf_index;
	terminal[current_terminal].current_prev = current_prev;
}

void load_kb(int current_terminal) {
	memcpy(kb_buf, terminal[current_terminal].kb_buf, KB_BUF_SIZE);
	memcpy(kb_prev_buf, terminal[current_terminal].kb_prev_buf, KB_BUF_SIZE*3);
	memcpy(kb_prev_buf_index, terminal[current_terminal].kb_prev_buf_index, 4*3);
	kb_buf_index = terminal[current_terminal].kb_buf_index;
	current_prev = terminal[current_terminal].current_prev;
}
