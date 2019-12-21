#include "paging.h"


/*
init_paging:
functionality: Initializes paging and allocates video and kernel memory
input:  None
outpu: None
Effects: paging system is implemented
*/
void init_paging(){
  int i;
  //initialize each page table entry,
  for(i = 0; i < PAGE_SIZE; i++){

      //set address offset, mark as writeable but not present
      page_table[i] = (i * FOUR_KB) | RW_SET;
      if (i >= V_MEM || i <= V_MEM+3) { //if we are at video memorie's location...
        page_table[i] |= RW_PRES_SET; //set  video memory to writeable and present
      }
  }

  //set the 4KB directory to present and writeable  and store the table base address
  page_dir[0] = ((unsigned int)page_table) | RW_PRES_SET;
  //set the kernel PDE to present and at address 4MB
  page_dir[1] = KERNEL_PAGE;


  //enable paging by setting control registers
  change_registers();
}


/*
init_paging:
functionality: Inline assembly function that sets control registers to enable
and properly handle paging
input:  None
outputs: None
Effects: control registers are altered to allow for paging
*/
void change_registers() {
  asm volatile(
              "movl %0, %%ebx;" // store page_dir location
              "movl %%ebx, %%cr3;" // move page_dir into cr3
              "movl %%cr4, %%ebx;" // move cr4 into temp reg
              "orl $0x00000010, %%ebx;" // set paging bits
              "movl %%ebx, %%cr4;" // store back into cr4
              "movl %%cr0, %%ebx;" // move into temp reg
              "orl $0x80000000, %%ebx;" // set paging bits
              "movl %%ebx, %%cr0;" // move back into cr0
              :                      /* no outputs */
              :"r" (page_dir)    /* input */
              :"%ebx"           /* clobbered register */
              );
}

/*
flush_TLB:
functionality: Clears the TLB by writing to it (taken from OSDEV)
input:  None
outputs: None
Effects: Translation Lookaside Buffer is flushed
*/
void flush_TLB() {
  asm volatile(
              "movl	%%cr3, %%eax;"      // store cr3
            	"movl	%%eax, %%cr3;"      // write to cr3
              :                      /* no outputs */
              :                      /* no input */
              : "eax"                /* clobbered register */
              );
}
/*
add_page:
functionality: maps a virtual address to a physical address
input: physical address -
      virtual address -
outputs: None
Effects: Maps a virtual address to a physical address
*/
void add_page(uint32_t physical_address, uint32_t virtual_address){
  // get the index of the directory to set
  int pd_index = virtual_address >> DIR_SHIFT & DIR_BITS;

  // set the page directory to store the loaction of the vid mem page table
  page_dir[pd_index] = (unsigned long)vmem_page_table | USR_WRITE_PRES;

  // create a new page entry to point to the correct location in physical
  vmem_page_table[0] = physical_address | USR_WRITE_PRES;

  // Flush TLB since we changed paging structure
  flush_TLB();
}
