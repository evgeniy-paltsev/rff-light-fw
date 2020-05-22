#include <soc.h>
#include <sys/printk.h>
#include <math.h>

#include "brightness-unified-model.h"


#define MAX_TIM_PWM		0xFFFF
#define MIN_TIM_PWM		0x0

#define MAX_OUT_VAL		(MAX_TIM_PWM * 2)
#define MID_OUT_VAL		0xEFF
#define OFF_OUT_VAL		MIN_TIM_PWM


static void set_sequential(uint32_t value, uint32_t *led0, uint32_t *led1)
{
	if (value > MAX_OUT_VAL)
		value = MAX_OUT_VAL;

	if (value < MAX_TIM_PWM) {
		if (led0)
			*led0 = value;
		if (led1)
			*led1 = MIN_TIM_PWM;
	} else {
		if (led0)
			*led0 = MAX_TIM_PWM;
		if (led1)
			*led1 = value - MAX_TIM_PWM;
	}
}

static void set_parallel(uint32_t value, uint32_t *led0, uint32_t *led1)
{
	if (value > MAX_OUT_VAL)
		value = MAX_OUT_VAL;

	value /= 2;

	*led0 = value;
	*led1 = value;
}

static void set_leds(uint32_t value, enum brightnes_geometry_ops geometry,
		     uint32_t *led0, uint32_t *led1)
{
	if (geometry == B_LED_PARALLEL)
		set_parallel(value, led0, led1);
	else
		set_sequential(value, led0, led1);
}

static uint32_t brightnes_ops_to_value(enum brightnes_value_ops ops)
{
	if (ops == BRIGHTNESS_OFF)
		return OFF_OUT_VAL;
	else if (ops == BRIGHTNESS_MID)
		return MID_OUT_VAL;
	else if (ops == BRIGHTNESS_MAX)
		return MAX_OUT_VAL;
	else
		/* we don't support BRIGHTNESS_THIS_OR_OFF yet */
		return OFF_OUT_VAL;
}

void brightness_log_xlate_to_2_auto(enum brightnes_value_ops brightnes_from,
				    enum brightnes_value_ops brightnes_to,
				    uint32_t brightnes_period,
				    uint32_t xlate_value,
				    uint32_t *led0, uint32_t *led1)
{
	uint32_t value_to = brightnes_ops_to_value(brightnes_to);

	if (brightnes_from == brightnes_to) {
		set_leds(value_to, B_LED_PARALLEL, led0, led1);

		return;
	}

	if (brightnes_period < 2) {
		set_leds(OFF_OUT_VAL, B_LED_PARALLEL, led0, led1);
		printk("WTF! %s:%d\n", __func__, __LINE__);

		return;
	}

	printk("%s:%d brightnes_period: %u, xlate_value: %u\n", __func__, __LINE__, brightnes_period, xlate_value);


	uint32_t value_from = brightnes_ops_to_value(brightnes_from);
	uint32_t max_output, min_output, curr_xlate, value;
	enum brightnes_geometry_ops geometry;

	if (xlate_value >= brightnes_period)
		xlate_value = brightnes_period - 1;

	printk("%s:%d brightnes_period: %u, xlate_value: %u, value_from: %x, value_to: %x\n", __func__, __LINE__, brightnes_period, xlate_value, value_from, value_to);

	if (value_from < value_to) {
		max_output = value_to;
		min_output = value_from;
		curr_xlate = xlate_value;
		geometry = B_LED_SEQUENTIAL;
	} else {
		max_output = value_from;
		min_output = value_to;
		curr_xlate = brightnes_period - xlate_value - 1;
		geometry = B_LED_PARALLEL;
	}

	printk("%s:%d curr_xlate: %u, xlate_value: %u, max_output: %x, min_output: %x\n", __func__, __LINE__, curr_xlate, xlate_value, max_output, min_output);

	/* TODO: cache const values */
	float log_const = ((brightnes_period - 1) * log10(2)) / (log10((max_output + 1) - min_output));
	value = min_output + round(pow(2, (curr_xlate / log_const))) - 1;

	printk("%s:%d value: %x\n", __func__, __LINE__, value);

	set_leds(value, geometry, led0, led1);
}

void brightness_log_xlate_to_2(enum brightnes_value_ops brightnes_from,
			       enum brightnes_value_ops brightnes_to,
			       enum brightnes_geometry_ops geometry,
			       uint32_t brightnes_period,
			       uint32_t xlate_value,
			       uint32_t *led0, uint32_t *led1)
{
	uint32_t value_to = brightnes_ops_to_value(brightnes_to);

	if (brightnes_from == brightnes_to) {
		set_leds(value_to, geometry, led0, led1);

		return;
	}

	if (brightnes_period < 2) {
		set_leds(OFF_OUT_VAL, geometry, led0, led1);
		printk("WTF! %s:%d\n", __func__, __LINE__);

		return;
	}


	uint32_t value_from = brightnes_ops_to_value(brightnes_from);
	uint32_t max_output, min_output, curr_xlate, value;

	if (xlate_value >= brightnes_period)
		xlate_value = brightnes_period - 1;

	if (value_from < value_to) {
		max_output = value_to;
		min_output = value_from;
		curr_xlate = xlate_value;
	} else {
		max_output = value_from;
		min_output = value_to;
		curr_xlate = brightnes_period - xlate_value - 1;
	}

	/* TODO: cache const values */
	float log_const = ((brightnes_period - 1) * log10(2)) / (log10((max_output + 1) - min_output));
	value = min_output + round(pow(2, (curr_xlate / log_const))) - 1;

	set_leds(value, geometry, led0, led1);
}
