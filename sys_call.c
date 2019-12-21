#include "sys_call.h"
#include "paging.h"
#include "filesystem.h"
#include "rtc.h"
#include "kb.h"
#include "types.h"
#include "x86_desc.h"
#include "terminal.h"

// counting in use processes
uint8_t num_active_blocks = 0;

// will hold current process number
uint8_t process_number;

// global starting address location for program pages (Set with bitmask)
 uint32_t start_address = START_ADDRESS; // 8MB + 10000111 set bits

// list of possible jump tables based on file type
fops rtc_fops = {rtc_open, rtc_close, rtc_read, rtc_write};
fops dir_fops = {dir_open, dir_close, dir_read, dir_write};
fops file_fops = {file_open, file_close, file_read, file_write};
fops stdin_fops = {kb_open_syscall, kb_close_syscall, kb_read_syscall, no_fops_func};
fops stdout_fops = {kb_open_syscall, kb_close_syscall, no_fops_func, kb_write_syscall};
fops no_fops_holder = {no_fops_func, no_fops_func, no_fops_func, no_fops_func};

/* halt
* Inputs: 8 bit value of halt status
* Outputs: None
* Side Effects: halts a currently running process and restores parent process data
*/
int32_t halt(uint8_t status) {
  int32_t i; // used for looping

  // clear interrupts while we do this
  cli();

  // grab current and parent pcb blocks
  pcb_t* curr = (pcb_t *)(STACK_START - (STACK_SIZE * (terminal[curr_terminal].curr_pid+1)));
  pcb_t* parent =  (pcb_t *)(STACK_START - (STACK_SIZE * (curr->parent_pid+1)));

  //set the currrent process to be freed
  current_processses_running[curr->curr_pid] = FREE;

  // for each file descriptor in the fd array of a process..
  for (i = 0; i < MAX_FILE_OPS; i++) {
    if(curr->fd_arr[i].file_flags != FREE) { // if the fd is in use, close it
        close(i);
    }
    // manually clear file descriptor fields
    curr->fd_arr[i].file_inode = NULL;
    curr->fd_arr[i].file_jumptable = no_fops_holder; // set to empty jump table

  }

    page_dir[_128MB] = START_ADDRESS + ((curr->parent_pid) * _4MB);

  // flush TLB since we have updated paging
  flush_TLB();

  // set tss to parent stack pointer
  tss.esp0 = curr->parent_stack_pointer; // -4 is for the tss struct and how it's stored
  terminal[curr_terminal].curr_pid = parent->curr_pid;
  terminal[curr_terminal].total_processes--;

  // if we are the last process, we do not want to close it, so re-run Shell
  if (curr->curr_pid == parent->curr_pid ) {//&& terminal[curr_terminal].total_processes == 0) {
    execute((uint8_t*)"shell");
  }


  sti();

  putc(NEWLINE); // want to leave a line between new shell input and last program output
  // restore parent processors stack and provides status value to the execute call

  asm volatile (
    "movl %0, %%ebp;" // move parent base pointer into ebp
    "movl %1, %%esp;" // move parent stack pointer into esp
    "movl %2, %%eax;" // move status into eax for function return
    "jmp IRET_RETURN;" // jump to after main IRET is complete but before it ends
    :
    : "r" (curr->parent_base_pointer), "r" (curr->parent_stack_pointer), "r" ((uint32_t)status)
    : "%eax"
  );

    // redudundant / unnecessary
    return GOOD;
}

/* execute
* Inputs: - * command - emulates a string of the command type
* Outputs: shouldn't hit the halt_return line, should ret through asm if successful ; return -1 for failure
* Side Effects: copies the appropriate data into the dentry struct based on if the file is found
*/
int32_t execute(const uint8_t * command) {
  // start of critical section

  cli();

  // used to determine if open fd or not
  uint8_t proc_flag = 0;

  // holds potential magic characters
  uint8_t exe_buf[SYS_BUF_SIZE];

  // will help find entry_point
  uint8_t entry_buf[SYS_BUF_SIZE];

  // temp dentry
  dentry_t dentry;

  // to be modified later
  int entry_point = 0;

  file_desc_t temp_fd[MAX_FILE_OPS];

  char temp_args[MAX_BYTES];

  uint32_t temp_child_proc, temp_base_pointer, temp_stack_pointer, temp_address;

  // ensuring a good command input
  if(command == NULL){
    return FAIL;
  }

  // handle command arg
  // first word is file
  // strip rest of spaces and store for getargs call
  uint8_t * args = (uint8_t *)command;
  int i = 0, j = 0, k = 0, halt_ret, m = 0;

  // move until we reach first word
  while (command[i] == ' ') {
    i++;
  }

  // finding start of first arg. breaking if a nullchar is found first
  // while (*((command+i)+j) != ' ' ||  *((command+i)+j) != '\0') {//|| *((command+i)+j) != '\0') {
  while (command[i+j] != ' ' &&  command[i+j] != '\0') {//|| *((command+i)+j) != '\0') {
    // if (*((command+i)+j) == '\0') break;
    j++;
  }

  // buffer to hold first word passed in
  uint8_t first_word[j+1];// = (uint8_t *)command;

  // append null character
  first_word[j] = '\0';


  // ifill first word buffer from overall command
  for (m = i; m < j; m++) {
    first_word[m] = command[m];
  }
  m = 0; // reset counter

  while (*(command+i+j+k) == ' ') { // check for leading spaces again for getargs arg
    k++;
  }

  // i is now at first non space of args
  args += (i+j+k); // shift pointer to the first character of args

  if(!(strncmp((const int8_t *)"exit", (const int8_t *)first_word, FOUR_B_OFFSET))) {

      asm volatile(
        "pushl $0;"
        "pushl $0;"
        "pushl %%eax;"
        "call halt;"
        :
      );
  }

  // check if file exists
  if(read_dentry_by_name((const uint8_t*)first_word, &dentry) == -1){
      return FAIL;
  }

  // return -1 if program does not exist or filename is not an executable
  if(read_data(dentry.inode_index, 0, exe_buf, SYS_BUF_SIZE) == -1){
    return FAIL;
  }

  // check that first 4 bytes of file are correct  0: 0x7f; 1: 0x45; 2: 0x4, 3: 0x46
  // fail if not
  if(exe_buf[0] != BYTE_ZERO && exe_buf[1] != BYTE_ONE && exe_buf[2] != BYTE_TWO && exe_buf[3] != BYTE_THREE){
    return FAIL;
  }

  // setting halt_ret val
  asm volatile (
    "movl %%eax, %0;"
    : "=r" (halt_ret)
  );

  read_data(dentry.inode_index, READ_SIZE, entry_buf, SYS_BUF_SIZE);
  // create a virtual address space for program
  // - create a new page directory for the program image
  // - single 4MB page mapping 0x08000000 to either 8MB or 12MB
  // - copy program to 0x00048000 within page

  // finding process number
  proc_flag = 0;
  for(i = 0; i < (MAX_FILE_OPS-SIX_FOPS_BEGIN); ++i) {
      if(current_processses_running[i] == IN_USE) {
          continue;
      }
      current_processses_running[i] = IN_USE;
      process_number = i;
      proc_flag = 1;
      break;
  }

  // no process available? return with error
  if(proc_flag == 0){
    return FAIL;
  }

  // getting current block using found process number
  pcb_t * curr_block;
  pcb_t * parent;
  // pcb_t * temp_block;

  if(terminal[curr_terminal].curr_pid > process_number && terminal[curr_terminal].total_processes > 0 ) {

      curr_block = (pcb_t *)(STACK_START - (STACK_SIZE * (terminal[curr_terminal].curr_pid+1)));
      parent = (pcb_t *)(STACK_START - (STACK_SIZE * (process_number+1)));

      current_processses_running[parent->curr_pid] = IN_USE;

      temp_child_proc = process_number;
      temp_base_pointer = parent->parent_base_pointer;
      temp_stack_pointer = parent->parent_stack_pointer;
      temp_address = parent->start_address;

      for(i = 0; i < MAX_FILE_OPS; ++i) {
          temp_fd[i] = parent->fd_arr[i];
          parent->fd_arr[i] = curr_block->fd_arr[i];
          curr_block->fd_arr[i] = temp_fd[i];
      }

      for(i = 0; i < MAX_BYTES; ++i) {
          temp_args[i] = parent->args_buf[i];
          parent->args_buf[i] = curr_block->args_buf[i];
          curr_block->args_buf[i] = temp_args[i];
      }

      curr_block->parent_pid = temp_child_proc;

      parent->curr_pid = temp_child_proc;
      parent->parent_pid = parent->curr_pid;

      parent->parent_base_pointer = curr_block->parent_base_pointer;
      curr_block->parent_base_pointer = temp_base_pointer;

      parent->parent_stack_pointer = curr_block->parent_stack_pointer;
      curr_block->parent_stack_pointer = temp_stack_pointer;

      parent->start_address = curr_block->start_address;

  }

  else {
    // maintaining current and parent process numbers
    curr_block = (pcb_t *)(STACK_START - (STACK_SIZE * (process_number+1)));

    curr_block->curr_pid = process_number;

    if(terminal[curr_terminal].total_processes == 0 || terminal[curr_terminal].curr_pid < 0)
    {
      curr_block->parent_pid = process_number;
      parent = (pcb_t *)(STACK_START - (STACK_SIZE * (process_number+1)));
    } else {
      parent = get_parent_pcb(terminal[curr_terminal].curr_pid);
      curr_block->parent_pid = parent->curr_pid;
    }
  }

  terminal[curr_terminal].curr_pid = curr_block->curr_pid;

  terminal[curr_terminal].total_processes++;

  // properly setting address of current block
  curr_block->start_address = (terminal[curr_terminal].curr_pid * _4MB) + START_ADDRESS;

  // Should be at 4MB offset depending on the PID
  page_dir[PAGE] = curr_block->start_address;

  // store arguments into pcb buffer
  for(m = 0; m < (MAX_BYTES - 1); m++){
    curr_block->args_buf[m] = args[m];
  }

  // FLUSH!
  flush_TLB();

  // get entry point, an unsigned int at bytes 24-27
  for(i = 0; i < AMOUNT_OF_BYTES; i++){
      entry_point |= (entry_buf[i] << (i*MAX_FILE_OPS));
  }

  // Copy the entire le to memory starting at virtual address 0x08048000
  read_data(dentry.inode_index, 0, (uint8_t *)VIRTUAL_ADDR, BIG_NUMBER);
  // jump to the entry point of the program to begin execution.
  //setup pcb

  // setting esp and ebp
  asm volatile(
               "movl %%esp, %%eax;"
               "movl %%ebp, %%ebx;"
               :"=a"(curr_block->parent_stack_pointer), "=b"(curr_block->parent_base_pointer)
              );
  // changing process numbers


  // setting default fd vals and accounting for stdin and stdout, which are always indices 0,1
  for(i = 0; i < MAX_FILE_OPS; i++){
    curr_block->fd_arr[i].file_inode = -1;
    curr_block->fd_arr[i].file_flags = FREE; // set rest
    curr_block->fd_arr[i].file_pos = 0;
    curr_block->fd_arr[i].file_jumptable = no_fops_holder;
  }
  curr_block->fd_arr[0].file_jumptable = stdin_fops;
  curr_block->fd_arr[1].file_jumptable = stdout_fops;
  curr_block->fd_arr[0].file_flags = IN_USE;
  curr_block->fd_arr[1].file_flags = IN_USE;

  // modifying tss values
  tss.ss0 = KERNEL_DS;
  tss.esp0 = STACK_START - (STACK_SIZE * terminal[curr_terminal].curr_pid) - TSS_OFFSET;// -4 is for the tss struct and how it's stored

  // end of critical section
  sti();


  // assembly code for context switching
  // virtual/fake IRET
  asm volatile(
               "cli;" // clear interrupts
               "movw %0, %%ax;" // move 16 bit user data segment selector to eax
               "movw %%ax, %%ds;" // move into data segment register
               "movl %1, %%eax;" // move user_esp into eax
               "pushl %0;" //push USER_DS
               "pushl %%eax;" //push user_esp
               "pushfl;" //push flags
               "popl %%edx;" // pop the user_esp into edx
               "orl %2, %%edx;" //or the user_esp with the IF_FLAG enabled
               "pushl %%edx;" //push the result of that IF_FLAG masking
               "pushl %3;" // push USER cs
               "pushl %4;" //push eip of the program
               "iret;"
               :  /* no outputs */
               :"i"(USER_DS), "r"(ESP_USER), "r"(IF_FLAG), "i"(USER_CS), "r"(entry_point)  /* input */
               : "cc", "memory", "%edx","%eax" /* clobbered register */
               );

  asm volatile (
              "IRET_RETURN:;" // label used by halt after it completes
              "LEAVE;"
              "RET;"
              );
  // should not get here
  return halt_ret;
}

/* read
* Functionality: reads data from file
* Inputs: file descriptor, buffer to copy into ,number of bytes to read,
* Outputs: number of bytes read if success, -1 for fail
* Side Effects:
*/
int32_t read(int32_t fd, void * buf, int32_t nbytes) {
  pcb_t* pcb = curr_pcb();

  if (fd < 0 || fd > MAX_FILE_OPS || buf == NULL || nbytes < 1) return FAIL;    // valid buf, nbytes, fd
  if (pcb->fd_arr[fd].file_flags == 0) return FAIL;

  int32_t retval = (pcb->fd_arr[fd].file_jumptable.read)(fd, buf, nbytes); // I think this is the syntax?

  retval+=0;

  return retval;

}
/* write
* Functionality: Write to a file
* Inputs: the file descriptor, the buffer to write, and number of bytes to write
* Outputs: -1 for failure, ottherwise the write system call
* Side Effects: none
*/
int32_t write(int32_t fd, const void * buf, int32_t nbytes) {
  pcb_t* pcb = (pcb_t *)(STACK_START - (STACK_SIZE * (terminal[curr_terminal].curr_pid+1)));
  if (fd < 0 || fd > MAX_FILE_OPS || buf == NULL || nbytes < 1) return FAIL;
  if (pcb->fd_arr[fd].file_flags == 0) return FAIL;
  // return the write system call
  return (pcb->fd_arr[fd].file_jumptable.write)(fd, buf, nbytes); // I think this is the syntax?
  // return kb_write_syscall(fd, buf, nbytes);
}
/* open
* Functionality: Opens a file
* Inputs: the file name that should open
* Output: index within the fd array for success, -1 for fail
* Side Effects:
*/
int32_t open(const uint8_t * filename) {
  pcb_t* pcb = curr_pcb();
  //file_desc_t* fd;
  dentry_t dentry;

  // find dentry
  // if does not exist ret -1
  int i = 0, flag = 0;
  if(strlen((const int8_t *)filename) == 0) return FAIL;
  if(read_dentry_by_name(filename, &dentry) == -1) return FAIL;

  // allocate a file descriptor
    // if none free ret -1
  for (i = SIX_FOPS_BEGIN; i < MAX_FILE_OPS; i++) {
    if (pcb->fd_arr[i].file_flags == FREE) {
      pcb->fd_arr[i].file_flags = IN_USE;
      pcb->fd_arr[i].file_pos = 0;
      flag = 1;
      break;
    }
  }

  if (!flag) return FAIL; // if none were free, return fail
  // set up data based on file type
  switch (dentry.filetype) {
    case RTC_TYPE:
      if (rtc_open(filename) != 0) return FAIL;
      pcb->fd_arr[i].file_jumptable = rtc_fops;
      pcb->fd_arr[i].file_inode = RTC_INODE;
      // fd->file_pos = ;
      break;
    case FOLDER_TYPE:
      // if (dir_open() != 0) return -1;
      pcb->fd_arr[i].file_jumptable = dir_fops;
      pcb->fd_arr[i].file_inode = -1;
      break;
    case FILE_TYPE:
      // if (file_open() != 0) return -1;
      pcb->fd_arr[i].file_jumptable = file_fops;
      pcb->fd_arr[i].file_inode = dentry.inode_index;
      break;
    default:
      return FAIL;
  }

    return i;
}
/* close
* Functionality: closes a file
* Inputs: a file descriptor
* Outputs: -1 for bad fd, 0 for valid close
* Side Effects: None
*/
int32_t close(int32_t fd) {
  pcb_t* pcb = curr_pcb();

  if (fd < SIX_FOPS_BEGIN || fd > MAX_FILE_OPS) return FAIL;   // valid file descriptor check
  if (pcb->fd_arr[fd].file_flags == FREE) return FAIL;
  pcb->fd_arr[fd].file_flags = FREE; // set flag to free
  return GOOD;
}

/* no_fops_func
* Functionality: Returns -1 always. Not done yet.
* Inputs: None
* Outputs: -1 always
*/
int32_t no_fops_func() {
    return FAIL;
}

/*
set_handler
* Functionality: Sys call that won't be implemented
* Inputs: signum - signal number
          handler_address - address of signal handler
* Outputs: returns -1 <- immediate failure
* Side Effects: none
*/
int32_t set_handler(int32_t signum, void * handler_address) {
    return FAIL;
}

/*
sigreturn
* Functionality: Sys call that won't be implemented
* Inputs: none
* Outputs: returns -1 <- immediate failure
* Side Effects: none
*/
int32_t sigreturn(void) {
    return FAIL;
}

/* curr_pcb
* Functionality: Helper function to get the current pcb
* Inputs: None
* Outputs: returns a pcb_t pointer to the current pcb
* Side Effects: As stated returns a pointer to the current pcb
*/
pcb_t *curr_pcb(void) {
  pcb_t *curr;
  asm volatile(
                "andl %%esp, %%eax;"
                :"=a"(curr)
                :"a"(PCB_MASK)
                :"cc"
              );

  return curr;
}

/* get_parent_pcb
* Functionality: Helper function to get the parent pcb
* Inputs: uint32_t parent_pid, holds the parent processor identifier - part of the pcb struct
* Outputs: the desired parent pcb
* Side Effects: As stated, parent pcb is found and returned as a pcb_t pointer
*/
pcb_t * get_parent_pcb(uint32_t parent_pid) {
  pcb_t * ret = (pcb_t *)(STACK_START - (parent_pid + 1) * STACK_SIZE);      //calculate
  return ret;
}
/*
vidmap:
functionality: maps the text mode video memory into user space
input: screen start -  Double pointer for the start of the screen
outputs: -1 for failure, 136 MB for all success
Effects: The screen_start pointer is adjusted, add_page is called and the virtual address
is mapped into physical memory
*/
int32_t vidmap (uint8_t** screen_start){
  // bad input check
  if (screen_start == NULL || screen_start == (uint8_t**)_4MB) return FAIL;
  // ensure screen start is within a valid range
  if (screen_start < (uint8_t **)__128MB || screen_start >= (uint8_t **)_132MB) {
  }

  add_page((uint32_t)PHYS_ADDR, (uint32_t)_136MB);      // map virtual address to physical space

  *screen_start = (uint8_t *)_136MB;          // set screen start pointer to virtual address

  return _136MB;                     // always return
}

/*
getargs:
functionality:
input: buf - will be written to with args
       nbytes - number of bytes to be copied
outputs: return -1 for failure, 0 for sucess
Effects: args written to buf's memory
is mapped into physical memory
*/
int32_t getargs(uint8_t* buf, int32_t nbytes) {

  // error check
  if (buf == NULL || nbytes+1 > MAX_BYTES) return FAIL;

  // loop counter
  int i;

  // get current pcb block
  pcb_t* cur = curr_pcb();

  //if no arguments are passed in, return bad on args
  if (cur->args_buf[0] == NULL) return FAIL;

  // go through arg buf of current pcb block and write it to the buf's memory
  for (i = 0; i < nbytes; i++) {
    buf[i] = cur->args_buf[i];
  }

  // return success
  return GOOD;
}
