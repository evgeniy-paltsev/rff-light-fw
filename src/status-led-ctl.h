#ifndef STSTUS_LED_CTL_H_
#define STSTUS_LED_CTL_H_

#include <soc.h>

void signal_led_init(void);
void led_gpio_enable(bool on);
void led_gpio_blink(u16_t on, u16_t off, u16_t count);

#endif
