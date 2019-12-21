#define PIT_MODE_3 0x36
#define PIT_COMMAND_REG 0x43
#define _20HZ 20
#define PIT_FREQ_MASK 0xFF
#define PIT_CHAN_0 0x40
#define FREQ_SHIFT 8
#define PIT_IRQ 0

// Initialize the PIT
extern void pit_init();
// Handle Interupts for the PIT
extern void pit_interrupt();

extern void switch_process(int pid, int next_pid);

int8_t cur_process_number;
int8_t next_process_number;
