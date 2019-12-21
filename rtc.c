//rtc.c file
#include "rtc.h"
#include "lib.h"
#include "i8259.h"
#include "tests.h"
volatile int rtc_interrupt_flag = 0;                         // globaly declare the interrupt flag as no interupt

/*
rtc_init:
functionality: RTC Interrupts are disabled by default, this will
enable & generate on IRQ 8
input:  None
outpu: None
Effects: RTC Interrupts are disabled by default, this will
enable & generate on IRQ 8
*/
void rtc_init(){
    outb(stat_reg_a, reg_num);      // select reg a
    unsigned char old_a = inb(write_CMOS);      // cur a value
    outb(stat_reg_b, reg_num);      // select reg b
    //unsigned char old_b = inb(0x71);
    outb(old_a | bit_six, write_CMOS);  // only 6 bit of reg b on
    enable_irq(irq_line);           // enable line 8

    // enable rtc interrupts, RTC_ON == 8
    enable_irq(irq_line);
}

/*
rtc_interrupt
Functionality: If register C is not read after an IRQ 8 then
the interrupt will not happen again
input: None
output: None
Effects: If register C is not read after an IRQ 8 then
the interrupt will not happen again
*/
void rtc_interrupt(){

      cli();      // Clear Interupt Flags
      outb(reg_c, reg_num);       // select reg c
      inb(write_CMOS);      //
      //test_interrupts();
      //printf("1");		// we need to print a 1 for every interupts
      send_eoi(irq_line);
      //printf("1");                              // added for rtc write test
      rtc_interrupt_flag = 1;                   // indicates we got an interuptope
      // sending end of interrupt signal
    // Allow Interupt flags
    sti();
}

//=============================================
//start of Checkpoint 2 functionality


/*
rtc_open
Functionality: opens rtc
input: filename - - not used but is part of template
output: retruns 0 to indicate success
Effects: rtc is open
*/
int32_t rtc_open(const uint8_t* filename){      // dosumentation says to pass in file, but it may be unneeded
    //rtc_init();                                 // init rtc, unsure if it is an issue to init more than once
    int32_t buffer = freq_2;                      // set the default frequency to 2
    return rtc_write(0,&buffer,max_rtc_bytes);
}
/*
rtc_read
Functionality: check if an interupt occuers, if an interupt occurs read rtc
input: fd, buf, nbytes - none are used ()
output: returns 0 to indicate a successful read
Effects: check if an interupt occuers, if an interupt occurs read rtc
*/
int32_t rtc_read(int32_t fd, void *buf, int32_t nbytes){
    while(rtc_interrupt_flag == 0){             //spin until we get a flag
                        // rtc_interrupt_flag is volitile so will contineu to check
    }

    rtc_interrupt_flag = 0;                     // re-set flag
    //printf("GOT TO END OF READ");
    return 0;
}
/*
rtc_write
Functionality:  Sets the new desired RTC Frequency
input:
    fd - not used ( template from sheet)
    buf - frequency we are going to use
    nytes - number of bytes; always four for rtc
output: -1 for invalid / FAIL
        0 for successful write
Effects: Sets the new desired RTC Frequency
*/
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    uint8_t rate;

    if(nbytes != max_rtc_bytes || buf == NULL){         // assure we have 4 bytes and the buffer is not null
        return -1;
    }

    uint32_t frequency = *((uint32_t*) buf);         // determine frequency
    outb(stat_reg_a, reg_num);       // hold  the old rtc data
    char previous = inb(write_CMOS);

    switch(frequency){                              // set the rate based on freq
        case freq_1024:
            rate = rate_freq_1024;
            break;
        case freq_512:
            rate = rate_freq_512;
            break;
        case freq_256:
            rate = rate_freq_256;
            break;
        case freq_128:
            rate = rate_freq_128;
            break;
        case freq_64:
            rate = rate_freq_64;
            break;
        case freq_32:
            rate = rate_freq_32;
            break;
        case freq_16:
            rate = rate_freq_16;
            break;
        case freq_8:
            rate = rate_freq_8;
            break;
        case freq_4:
            rate = rate_freq_4;
            break;
        case freq_2:
            rate = rate_freq_2;
            break;
        case freq_0:
            rate = rate_freq_0;
            break;
        default:
            return -1;
    }

    outb(stat_reg_a, reg_num);
    char temp = (previous & first_half_mask);       // We want first half
    temp = (temp | rate);                           // combine values
    outb(temp, write_CMOS);                           // write new freq
    //printf("GOT TO END OF WRITE");
    return 0;
}
/*
rtc_close
Functionality: Closes RTC /
input: None
output: int32_t 0 always
Effects: Closes RTC
*/
int32_t rtc_close(int32_t fd){
    return 0; //does nothing
}
