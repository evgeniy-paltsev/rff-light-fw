#ifndef BRIGHTNESS_MODEL_H_
#define BRIGHTNESS_MODEL_H_

#include <soc.h>
#include <sys/printk.h>
#include <math.h>

struct brightness_model {
	uint32_t (*xlate)(uint32_t value);
//	void (*xlate_leds)(uint32_t value, uint32_t *led0, uint32_t *led1);
	void (*xlate_leds)(struct brightness_model *model, uint32_t value, uint32_t *led0, uint32_t *led1);

	/* private */
	float		g_model_logarithmic_const;
	uint32_t	g_max_in_value;
};

void model_logarithmic_init(struct brightness_model *model, uint32_t max_in_value);

#endif
