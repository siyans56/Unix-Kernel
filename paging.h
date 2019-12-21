#ifndef PAGING_H
#define PAGING_H

#include "types.h"
#include "lib.h"

#define PAGE_SIZE 1024 // size of our tables
#define FOUR_KB 4096 // 4 KB in binary size
#define ADDR_SHIFT 12 // shift used to move addresses in page entries
#define V_MEM 0xB8 // location of video memory in pages
#define RW_SET 0x00000002 // bitset for write mode, NOT PRESENT
#define RW_PRES_SET 3 // bitset for write mode + present
#define PSE_FLAG 0x10 // setting the PSE flag
#define PG_FLAG 0x80000001 // setting the PG Flag
#define NUM_DIRS 64 // number of directory entries we have
#define KERNEL_PAGE 0x400083 // bits for 4MB Directory page
#define DIR_SET 0x3 // bits for other directory page
#define FOUR_MB 0x400000 // bits for 4MB in hex
#define DIR_SHIFT 22 // bit shift to store directory offset
#define DIR_BITS 0x03FF // bits set for a directory entry
#define USR_WRITE_PRES 7 // bits to set to user, writeable, and present


// array of page directory entries
uint32_t page_dir[NUM_DIRS] __attribute__((aligned(FOUR_KB))); //set both to present and R/W

//array of page table entries
uint32_t page_table[PAGE_SIZE] __attribute__((aligned(FOUR_KB)));

uint32_t vmem_page_table[PAGE_SIZE] __attribute__((aligned(FOUR_KB)));

// 2 Directories: One for the table holding the 1024 4KBs, one for the 4MB page (4MB does not use tables)
// 1 table: 1024 entries for each 4KB page in one,

//initialize paging
extern void init_paging();
//inline assembly helper to init paging
extern void change_registers();
//inline assembly helper to flush the Translation Lookaside Buffer
extern void flush_TLB();
// add another page mapping for the program
extern void add_page(uint32_t physical_address, uint32_t virtual_address);
#endif


/*
QUESTIONS:
How large is video memory? 4KB
*/

/*
REQUIREMENTS:
- Must load a page directory and table into memory
- Load PDBR / CR3 register with physical base addr of page directory (do before setting PG flag)
- mark all memory as not present if unused (not video memory or kernel) DONE

LAYOUT:
- for 0-4 MB in virtual/physical use 4KB pages
- for 4-8 MB in virtual/physical use 4MB page (only 10 MSB for base address)

- page directory: An array of 32-bit page-directory entries (PDEs) contained in a 4-KByte
page.
- page table: An array of 32-bit page-table entries (PTEs) contained in a 4-KByte page
- page: A 4-KByte or 4-MByte flat address space
- PDE (4KB): SEE 3-15 for 4MB version
  - bits 31-12: PTE base address
  - bits 9-11: user available
  - bit 8: Global page (ignored)
  - bit 7: Page Size (PS)
    - if PS = 1: PDE points to a 4MB page (PSE must be set to 1 also)
    - if PS = 0: PDE points to a 4KB page
  - bit 6: Set to 0
    - 0 if not written to before
    - 1 if written to before
  - bit 5: Accessed (A): controlled by software
    - 0 if not accessed
    - 1 if read/written to
  - bit 4: Cache disabled (PCD)
    - 0 to allow page caching
    - 1 to prevent page caching
  - bit 3: Write-Through (PWT)
    - 0 to enable write-back caching
    - 1 to disable
  - bit 2: User/Supervisor (u/S)
    - 0 if Supervisor
    - 1 if User
  - bit 1: Read/Write (R/W)
    - 0 if read only
    - 1 if writeable
  - bit 0: Present (P)
    - 0 if page/table is not in memory
    - 1 if page/table is in memory
- PTE:
  - bits 31-12: page base address
  - bits 9-11: user available
  - bit 8: Global page (ignored)
  - bit 7: Page Table Attribute Index
    - keep to 0
  - bit 6: Set to 0
    - 0 if not written to before
    - 1 if written to before
  - bit 5: Accessed (A): controlled by software
    - 0 if not accessed
    - 1 if read/written to
  - bit 4: Cache disabled (PCD)
    - 0 to allow page caching
    - 1 to prevent page caching
  - bit 3: Write-Through (PWT)
    - 0 to enable write-back caching
    - 1 to disable
  - bit 2: User/Supervisor (u/S)
    - 0 if Supervisor
    - 1 if User
  - bit 1: Read/Write (R/W)
    - 0 if read only
    - 1 if writeable
  - bit 0: Present (P)
    - 0 if page/table is not in memory
    - 1 if page/table is in memory


TRANSLATION (given a linear address):
- bits 22-31: offset to PDE
- bits 12-21: offset to PTE
- bits 0-11: offset to physical address
- for 4 MB, bits 0-21 are all offsets to physical addresses

NOTES:
Paging allows the translation of linear addresses to physical addresses
PG flag in CR0: if 0 off. Before setting:

If the page containing the linear address is not currently in physical memory, the processor
generates a page-fault exception (#PF).

Paging is controlled by three flags in the processor’s control registers:
• PG (paging) flag. Bit 31 of CR0
  - enables page translation. Set during initialization
• PSE (page size extensions) flag. Bit 4 of CR4
  - enables 4MB page

SEE TABLE 3-3/FIG 3-12 FOR TRANSLATION

*/
