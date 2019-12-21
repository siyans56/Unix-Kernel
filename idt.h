// initializing the IDT
extern void idt_init();

// macro for a system call vector value
#define SYS_CALL_VEC 0x80

// 3 is lowest priority level
#define USER_LVL 0x3

// 0 is highest priority level
#define KERNEL_LEVEL 0x0
