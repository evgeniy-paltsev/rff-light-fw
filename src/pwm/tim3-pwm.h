#ifndef TIM_3_CONTROL
#define TIM_3_CONTROL

#include <soc.h>

void timer3_pwm_init(void);
uint32_t get_curr_pwm_value(void);

#endif
