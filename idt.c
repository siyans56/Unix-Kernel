#include "idt.h"
#include "x86_desc.h"
#include "lib.h"
#include "kb.h"
#include "rtc.h"
#include "interruptHandler.h"
#include "sys_call.h"

#define NUM_NONGENERAL_INTERRUPTS 32
#define USER_DPL 0x3

/* Divide_Error
* Inputs: none
* Outputs: none
* Side Effects: displays that divide by zero exception occurred, loops forever
*/
void Divide_Error(){
    cli();
    printf("  Divide Error\n");
    while(1) {}
    sti();
}

/* Reserved
* Inputs: none
* Outputs: none
* Side Effects: displays that reserved exception occurred, loops forever
*/
void Reserved(){
    cli();
    printf("  Reserved/Debug\n");
    while(1) {}
    sti();
}

/* NMI_Interrupt
* Inputs: none
* Outputs: none
* Side Effects: displays that nmi exception occurred, loops forever
*/
void NMI_Interrupt(){
    cli();
    printf("  NMI_Interrupt\n");
    while(1);
    sti();
}

/* Breakpoint
* Inputs: none
* Outputs: none
* Side Effects: displays that divide by breakpoint occurred, loops forever
*/
void Breakpoint(){
    cli();
    printf("  Breakpoint\n");
    while(1);
    sti();
}

/* Overflow
* Inputs: none
* Outputs: none
* Side Effects: displays that overflow exception occurred, loops forever
*/
void Overflow(){
    cli();
    printf("  Overflow\n");
    while(1);
    sti();
}

/* Bound
* Inputs: none
* Outputs: none
* Side Effects: displays that bound exception occurred, loops forever
*/
void Bound(){
    cli();
    printf("  Bound\n");
    while(1);
    sti();
}

/* Invalid_Opcode
* Inputs: none
* Outputs: none
* Side Effects: displays that invalid opcode exception occurred, loops forever
*/
void Invalid_Opcode(){
    cli();
    printf("  Invalid_Opcode\n");
    while(1);
    sti();
}

/* Device_NA
* Inputs: none
* Outputs: none
* Side Effects: displays that device unavailable exception occurred, loops forever
*/
void Device_NA(){
    cli();
    printf("  Device_NA\n");
    while(1);
    sti();
}

/* Double_Fault
* Inputs: none
* Outputs: none
* Side Effects: displays that double fault exception occurred, loops forever
*/
void Double_Fault(){
    cli();
    printf("  Double_Fault\n");
    while(1);
    sti();
}

/* Segment_Overrun
* Inputs: none
* Outputs: none
* Side Effects: displays that segment overrun exception occurred, loops forever
*/
void Segment_Overrun(){
    cli();
    printf("  Segment_Overrun\n");
    while(1);
    sti();
}

/* invalid_tss
* Inputs: none
* Outputs: none
* Side Effects: displays that invalid tss exception occurred, loops forever
*/
void invalid_tss() {
    cli();
    printf("  Invalid TSS Error\n");
    while(1);
    sti();
}

/* seg_not_present
* Inputs: none
* Outputs: none
* Side Effects: displays segment not present exception occurred, loops forever
*/
void seg_not_present() {
    cli();
    printf("  Segment Not Present\n");
    while(1);
    sti();
}

/* stack_seg_fault
* Inputs: none
* Outputs: none
* Side Effects: displays that stack seg fault occurred, loops forever
*/
void stack_seg_fault() {
    cli();
    printf("  Stack-Segment Fault\n");
    while(1);
    sti();
}

/* general_protection
* Inputs: none
* Outputs: none
* Side Effects: displays that general protection exception occurred, loops forever
*/
void general_protection() {
    cli();
    printf("  General Protection Error\n");
    halt(255);
    sti();
}

/* page_fault
* Inputs: none
* Outputs: none
* Side Effects: displays that page fault exception occurred, loops forever
*/
void page_fault() {
    cli();
    printf("  Page fault\n");
    halt(255);
    sti();
}

/* floating_point_error
* Inputs: none
* Outputs: none
* Side Effects: displays that floating point exception occurred, loops forever
*/
void floating_point_error() {
    cli();
    printf("  Floating Point Error\n");
    while(1);
    sti();
}

/* align_check
* Inputs: none
* Outputs: none
* Side Effects: displays that align check exception occurred, loops forever
*/
void align_check() {
    cli();
    printf("  Alignment Check Error\n");
    while(1);
    sti();
}

/* machine_check
* Inputs: none
* Outputs: none
* Side Effects: displays that machine check exception occurred, loops forever
*/
void machine_check() {
    cli();
    printf("  Machine Check Error\n");
    while(1);
    sti();
}

/* simd_floating_point_exception
* Inputs: none
* Outputs: none
* Side Effects: displays that simd floating point exception occurred, loops forever
*/
void simd_floating_point_exception() {
    cli();
    printf("  SIMD Floating Point Exception\n");
    while(1);
    sti();
}

/* general
* Inputs: none
* Outputs: none
* Side Effects: displays that general interrupt occurred (not Intel 0-31 in IDT)
*/
void general() {
    cli();
    printf("  General Interrupt\n");
    sti();
}

void syscall() {
    cli();
    printf("  SYSTEM CALL\n");
    sti();
}

/* idt_init
* Inputs: none
* Outputs: none
* Side Effects: Loads idt_desc_ptr, initializes values based off trap and interrupt gate
  documentation. Also sets specific exceptions for idt table [0,19]
*/
void idt_init(){

    // loop counter
    int i;

    // loads idt_desc_ptr
    lidt(idt_desc_ptr);


    // NUM_VEC == 256. set values for bits of each element in IDT
    for(i = 0; i < NUM_VEC; i++){

         // default values of interrupt gate based of intel docs
        idt[i].reserved4 = 0x0;
        idt[i].reserved3 = 0x0;
        idt[i].reserved2 = 0x1;
        idt[i].reserved1 = 0x1;
        idt[i].reserved0 = 0x0;
        idt[i].size = 0x1;
        idt[i].dpl = 0x0;
        idt[i].present = 0x1;
        idt[i].seg_selector = KERNEL_CS;

        // general interrupt for every IDT elem unless later specified
        SET_IDT_ENTRY(idt[i], general);

        // if interrupt 0x80, handle a system call by setting level to user and indicating trap
        if(i == SYS_CALL_VEC){
            idt[i].dpl = USER_DPL;
            idt[i].reserved3 = 0x1;
        }

        // interrupts [0,31] are traps, hence reserved3 must be 1 (according to system)
        if(i < NUM_NONGENERAL_INTERRUPTS){
            idt[i].reserved3 = 0x1;
        }
    }

    // handling exceptions [0,19], calling respective function to display occurred exception
    SET_IDT_ENTRY(idt[0], Divide_Error);
    SET_IDT_ENTRY(idt[1], Reserved);
    SET_IDT_ENTRY(idt[2], NMI_Interrupt);
    SET_IDT_ENTRY(idt[3], Breakpoint);
    SET_IDT_ENTRY(idt[4], Overflow);
    SET_IDT_ENTRY(idt[5], Bound);
    SET_IDT_ENTRY(idt[6], Invalid_Opcode);
    SET_IDT_ENTRY(idt[7], Device_NA);
    SET_IDT_ENTRY(idt[8], Double_Fault);
    SET_IDT_ENTRY(idt[9], Segment_Overrun);
    SET_IDT_ENTRY(idt[10], invalid_tss);
    SET_IDT_ENTRY(idt[11], seg_not_present);
    SET_IDT_ENTRY(idt[12], stack_seg_fault);
    SET_IDT_ENTRY(idt[13], general_protection);
    SET_IDT_ENTRY(idt[14], page_fault);
    SET_IDT_ENTRY(idt[16], floating_point_error);
    SET_IDT_ENTRY(idt[17], align_check);
    SET_IDT_ENTRY(idt[18], machine_check);
    SET_IDT_ENTRY(idt[19], simd_floating_point_exception);

    // keyboard int to read char, is taken to interruptHandler.S
    SET_IDT_ENTRY(idt[33], keyboard_INT);

    // rtc int - is taken to interruptHandler.S
    SET_IDT_ENTRY(idt[40], rtc_INT);
    SET_IDT_ENTRY(idt[SYS_CALL_VEC], sys_call_INT);

    SET_IDT_ENTRY(idt[32], pit_INT);

}
