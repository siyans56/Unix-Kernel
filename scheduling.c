#include "sys_call.h"
#include "paging.h"
#include "filesystem.h"
#include "rtc.h"
#include "kb.h"
#include "types.h"
#include "x86_desc.h"
#include "scheduling.h"
#include "i8259.h"
#include "terminal.h"

// Use Equal Time Slices Round Robin
// Test with counter, Ping Pong, fish
// be mindful of Synchronization Issues
// utilize the kernal stack

/* pit_init
* Functionality: initalizes the PIT (Programmable Interval Timer), enables interupts on
* PIC which will allow for scheduling
* Inputs: None
* Outputs: None
* Side Effects: Interupts are now enabled
*/
void pit_init(void){
  // // printf("weewoo\n");
  //   // outb(PIT_MODE_3, PIT_COMMAND_REG);           // Set int freq to 20HZ
  //   outb(0x36, 0x43);           // Set int freq to 20HZ
  //
  //   //CHECK THE 20 HZ NUMBER
  //   // outb(_20HZ & PIT_FREQ_MASK, PIT_CHAN_0);     // NOTE: 20 HZ may be too laggy
  //   outb(20 & 0xFF, 0x40);     // NOTE: 20 HZ may be too laggy
  //
  //   // outb(_20HZ >> FREQ_SHIFT, PIT_CHAN_0);       //
  //   outb(20 >> 8, 0);       //
  //
    cur_process_number = terminal[0].curr_pid;//curr->curr_pid;        // Get process Number of the next process to be executed
    next_process_number = 0;// terminal[1].curr_pid;//(process_number+1)%8;     // Next process number, increment 1, mod 8
  //
  //
  // printf("done\n");

    // Firstly, register our timer callback.
    enable_irq(PIT_IRQ);                         // Enable PIT ints on line 0
    // register_interrupt_handler(IRQ0, &timer_callback);

    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency. Important to note is
    // that the divisor must be small enough to fit into 16-bits.
    uint32_t divisor = 1193180 / _20HZ;

    // Send the command byte.
    outb(0x36, 0x43);

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor.
    outb(l, 0x40);
    outb(h, 0x40);


}
/* pit_interupt
* Functionality: Handles PIT interupts, an int to the PIT requires scheduling to occur
* Context Switching and what not
* Inputs: None
* Outputs: None
* Side Effects: Switches Process that is occuing
*/
void pit_interrupt(void){

    printf("int ");
    send_eoi(PIT_IRQ);            // end the cur int before we update

    // cli();                  // Start: Mask Interrupts
    // send_eoi defined in i8259 for refrence

    // pcb_t* curr = curr_pcb();                // Get process Control Block. How we found Process number in sys_call
    // uint8_t process_number = terminal[0].curr_pid;//curr->curr_pid;        // Get process Number of the next process to be executed
    // uint8_t next_process_number = terminal[]//(process_number+1)%8;     // Next process number, increment 1, mod 8


    // go thru each terminal,

    // if(cur_process_number == next_process_number){              // No more scheduling to be done
    //   return;                                             // return as a result
    // }
    // Update the next process number until it remains constant for one iteration

    if (cur_process_number == -1) {
      cur_process_number = 0;
      execute((uint8_t*)"shell");
      return;
    }

    next_process_number = (next_process_number+1) % 6;
    while (current_processses_running[0] == FREE) {
      next_process_number = (next_process_number+1) % 6;
    }


    // next_process_number = terminal[running_terminal+1 % 3].curr_pid; //(next_process_number + 1) % 6;        //Update the next process number
    switch_process(cur_process_number, next_process_number);
    cur_process_number = next_process_number;                   // update cur process Num, next was already updated
    // while(process_number != next_process_number){
    //
    // }




    // // Iterate until only one process is running
    // while(process_number != next_process_number){
    //     next_process_number = (next_process_number + 1) % 6;        //Update the next process number continuously
    // }
    // if(process_number == next_process_number){              // If there is only one process number we can exit interupts
    //     return;
    // }
    // process_number = next_process_number;               //update cur process Num



    // sti();                  // End: UnMask Interrupts

    /*
    Should have the correct Process Number & Next process Num at this point, this is where it starts to not make much sense

    refrence how Siyan did ESP/EBP swapping in terminal.c

    not sure if that swap is enough

    may need to do some vid map stuff


    */
}

void switch_process(int pid, int next_pid) {
  // save current ebp/esp
  pcb_t * curr_pcb = get_parent_pcb(pid);
  pcb_t * next_pcb = get_parent_pcb(next_pid);
  asm volatile(
               "movl %%esp, %%eax;"
               "movl %%ebp, %%ebx;"
               :"=a"(curr_pcb->stack_pointer), "=b"(curr_pcb->base_pointer)
              );

  // switch process paging ??
  printf("running terminal: %d", running_terminal);
  running_terminal = running_terminal+1 % 3; // set running terminal to the next terminal
  printf("next terminal: %d", running_terminal);


  // set tss to next process
  tss.esp0 = STACK_START - (STACK_SIZE * next_pid) - TSS_OFFSET;// -4 is for the tss struct and how it's stored


  // update vidmem
  // not needed

  // load next process ebp / esp
  asm volatile (
                "movl %0, %%ebp;" // move parent base pointer into ebp
                "movl %1, %%esp;" // move parent stack pointer into esp
                :
                :"r"(next_pcb->stack_pointer), "r"(next_pcb->base_pointer)

              );

  flush_TLB();

}
