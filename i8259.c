/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* i8259_init
* Inputs: void
* Outputs: None
* Side Effects: initializes pic by writing to ICW1-ICW4
*/
void i8259_init(void) {

  // write to ports for ICW1-4
  outb(ICW1, MASTER_8259_PORT);
  outb(ICW1, SLAVE_8259_PORT);
  outb(ICW2_MASTER, MASTER_PORT2);
  outb(ICW2_SLAVE, SLAVE_PORT2);
  outb(ICW3_MASTER, MASTER_PORT2);
  outb(ICW3_SLAVE, SLAVE_PORT2);
  outb(ICW4, MASTER_PORT2);
  outb(ICW4, SLAVE_PORT2);

  // enables irq for PIC (2)
  enable_irq(PIC_IRQ);

}

/* enable_irq
* Inputs: void
* Outputs: None
* Side Effects: enables irqs for all inits
*/
void enable_irq(uint32_t irq_num) {
  uint16_t port;

  // irq_number is valid if it's between 0 and 15
  if(irq_num < LOW_BOUND || irq_num > HIGH_BOUND) {
    return;
  }

  // master
  if(irq_num < MID_BOUND && irq_num >= LOW_BOUND){
    port = MASTER_PORT2; // get the correct port
    master_mask = master_mask & ~(IRQ_BMASK << irq_num); //mask the master_mask based on the irq_number given
    outb(master_mask, port);

  }

  // slave
  else if(irq_num >= MID_BOUND && irq_num < HIGH_BOUND){
    port = SLAVE_PORT2; // get the correct port
    irq_num -= MID_BOUND; // get the irq number on the slave pic
    slave_mask = slave_mask & ~(IRQ_BMASK << irq_num); //mask the slave_mask based on the irq_number given
    outb(slave_mask, port);
  }


}

/* disable_irq
* Inputs: void
* Outputs: None
* Side Effects: disables irqs for all inits
*/
void disable_irq(uint32_t irq_num) {
  uint16_t port;

  // irq_number is valid if it's between 0 and 15
  if(irq_num < LOW_BOUND || irq_num > HIGH_BOUND) {
    return;
  }

  // master
  if(irq_num < MID_BOUND && irq_num >=LOW_BOUND){
    port = MASTER_PORT2;
    master_mask = master_mask | (IRQ_BMASK << irq_num); //mask the master_mask based on the irq_number given
    outb(master_mask, port);

  }

  // slave
  else{
    port = SLAVE_PORT2;
    irq_num -= HIGH_BOUND; // adjust irq_num to get the irq num for the slave
    slave_mask = slave_mask | (IRQ_BMASK << irq_num); //mask the slave_mask based on the irq_number given
    outb(slave_mask, port);
  }
}

/* send_eoi
* Inputs: void
* Outputs: None
* Side Effects: sends end of interrupt signal
*/
void send_eoi(uint32_t irq_num) {

    // slave
    if(irq_num >= MID_BOUND && irq_num <= HIGH_BOUND){
      outb(EOI | (irq_num - MID_BOUND), SLAVE_8259_PORT);
      outb(EOI + 2, MASTER_8259_PORT); // add
    }

    // master
    else if(irq_num >= LOW_BOUND && irq_num < MID_BOUND){
      outb(EOI | irq_num, MASTER_8259_PORT);
    }
}
