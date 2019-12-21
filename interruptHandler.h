
// implemented in interruptHandler.S
extern void rtc_INT();
//keyboard interrupt handler
extern void keyboard_INT();
//syscall interrupt handler
extern void sys_call_INT();
// PIT interrupt handler
extern void pit_INT();
