//rtc.h
#include "types.h"
#include "lib.h"

// to be implemented in rtc.c
extern void rtc_init();
extern void rtc_interrupt();
int32_t rtc_open (const uint8_t* filename);
int32_t rtc_read (int32_t fd, void *buf, int32_t nbytes);
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_close (int32_t fd);
//volatile int rtc_interrupt_flag; // inidicates if an int is occuring, must be volitile due to


// used for testing only
// static int test_count = 0;

//  specific port values for rtc init and interrupt handling
#define stat_reg_a 0x8A
#define stat_reg_b 0x8B
#define reg_num 0x70
#define write_CMOS 0x71
#define bit_six 0x40
#define irq_line 8
#define reg_c 0x0C
#define rtc_data 0x71
#define first_half_mask 0xF0
#define freq_1024 1024
#define freq_512 512
#define freq_256 256
#define freq_128 128
#define freq_64 64
#define freq_32 32
#define freq_16 16
#define freq_8 8
#define freq_4 4
#define freq_2 2
#define freq_0 0
#define rate_freq_1024 0x06
#define rate_freq_512 0x07
#define rate_freq_256 0x08
#define rate_freq_128 0x09
#define rate_freq_64 0x0A
#define rate_freq_32 0x0B
#define rate_freq_16 0x0C
#define rate_freq_8 0x0D
#define rate_freq_4 0x0E
#define rate_freq_2 0x0F
#define rate_freq_0 0x00
#define max_rtc_bytes 4
