#ifndef TIMER_H
#define TIMER_H

#define TIMER_INT     0x08      /* Timer interrupt */
#define PIT_FREQUENCY 1193181L  /* Frequency of PIT */

void set_timer (void (far interrupt *handler)(void), unsigned short freq);
void restore_timer (void);
void timer_init (void);

#endif /* TIMER_H */
