/*
 * Timer routines
 *
 */
#include <dos.h>
#include <conio.h>
#include "timer.h"

/*
 * Globals
 */
void interrupt new_handler (void);
void (interrupt far *old_handler) (void);

/*
 * These help keeping the system time up correctly
 */
long clock_ticks;
long counter;

void
timer_init (void)
{
       set_timer (&new_handler, 30);  /* 30 Hz frequency */
}


void
reset_timer (void)
{
       restore_timer();
}

/*
 * Timer interrupt (int 8) handler
 */
void interrupt far
new_handler (void)
{
       /*
        * Update the clock tick counter and then call
        * the original timer handler if that's necessary.
        */
       clock_ticks += counter;
       
       if (clock_ticks >= 0x10000) {
              clock_ticks -= 0x10000;
              old_handler();
       } else
              outportb(0x20, 0x20); /* Send EOI signal to PIC */
}


void
set_timer (void (far interrupt *handler)(void), unsigned short freq)
{
       clock_ticks = 0;
       counter = PIT_FREQUENCY / freq;

       old_handler = getvect (TIMER_INT);
       setvect (TIMER_INT, handler);

       outportb (0x43, 0x34);
       outportb (0x40, counter & 0xff);        /* LSB */
       outportb (0x40, (counter >> 8) & 0x00ff); /* MSB */
}


void
restore_timer (void)
{
       outportb (0x43, 0x34);
       outportb (0x40, 0);
       outportb (0x40, 0);
       setvect (TIMER_INT, old_handler);
}
