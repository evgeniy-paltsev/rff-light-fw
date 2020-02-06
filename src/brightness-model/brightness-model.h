#ifndef BRIGHTNESS_MODEL_H_
#define BRIGHTNESS_MODEL_H_

#include <soc.h>
#include <sys/printk.h>
#include <math.h>

struct brightness_model {
	void (*init)(uint32_t max_value);
	uint32_t (*xlate)(uint32_t value);
	void (*xlate_leds)(uint32_t value, uint32_t *led0, uint32_t *led1);
};

struct brightness_model *get_model_logarithmic(void);
//struct brightness_model *get_model_s_curve_table(void);

#endif
