#include "brightness-model.h"

#define TWEEK_VAL		10

#define MAX_TIM_PWM		0xFFFF
#define MAX_OUT_VAL		(MAX_TIM_PWM * 2)

static float g_model_logarithmic_const = 1;
static uint32_t g_max_in_value = 1;

//static float log_base(float x, float base) {
//	return log(x) / log(base);
//}

static void model_logarithmic_init(uint32_t max_in_value)
{
	g_model_logarithmic_const = (max_in_value * log10(2)) / (log10(MAX_OUT_VAL));
	g_max_in_value = max_in_value;
}

static uint32_t model_logarithmic_xlate(uint32_t in_value)
{
	uint32_t value;

	if (in_value > g_max_in_value)
		in_value = g_max_in_value;

	value = round(pow(2, (in_value / g_model_logarithmic_const)));

	if (value > MAX_OUT_VAL)
		value = MAX_OUT_VAL;

	return value;
}

static void model_logarithmic_xlate_leds(uint32_t in_value, uint32_t *led0, uint32_t *led1)
{
	uint32_t value;

	if (in_value > g_max_in_value)
		in_value = g_max_in_value;

	value = round(pow(2, (in_value / g_model_logarithmic_const)));

	if (value > MAX_OUT_VAL)
			value = MAX_OUT_VAL;

	if (value < MAX_TIM_PWM) {
		if (led0)
			*led0 = value;
		if (led1)
			*led1 = 0;
	} else {
		if (led0)
			*led0 = MAX_TIM_PWM;
		if (led1)
			*led1 = value - MAX_TIM_PWM;
	}
}

struct brightness_model brightness_model_logarithmic = {
	.init = model_logarithmic_init,
	.xlate = model_logarithmic_xlate,
	.xlate_leds = model_logarithmic_xlate_leds,
};

struct brightness_model *get_model_logarithmic(void)
{
	return &brightness_model_logarithmic;
}

/*
const uint16_t br[] = {
  0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9,
  9, 10, 10, 11, 12, 12, 13, 13, 14, 15, 16, 17, 17, 18, 19,
  20, 21, 22, 24, 25, 26, 28, 29, 30, 32, 34, 35, 37, 39, 41,
  43, 46, 48, 50, 53, 55, 58, 61, 64, 67, 71, 74, 78, 82, 86,
  90, 94, 99, 104, 109, 114, 119, 125, 130, 137, 143, 149,
  156, 163, 170, 178, 186, 194, 202, 210, 219, 228, 238, 247,
  257, 267, 278, 288, 299, 310, 322, 333, 345, 357, 369, 382,
  394, 407, 420, 433, 446, 459, 472, 485, 499, 512, 525, 539,
  552, 565, 578, 591, 604, 617, 630, 642, 655, 667, 679, 691,
  702, 714, 725, 736, 746, 757, 767, 777, 786, 796, 805, 814,
  822, 830, 838, 846, 854, 861, 868, 875, 881, 887, 894, 899,
  905, 910, 915, 920, 925, 930, 934, 938, 942, 946, 950, 953,
  957, 960, 963, 966, 969, 971, 974, 976, 978, 981, 983, 985,
  987, 989, 990, 992, 994, 995, 996, 998, 999, 1000, 1002,
  1003, 1004, 1005, 1006, 1007, 1007, 1008, 1009, 1010, 1011,
  1011, 1012, 1012, 1013, 1014, 1014, 1015, 1015, 1016, 1016,
  1016, 1017, 1017, 1017, 1018, 1018, 1018, 1019, 1019, 1019,
  1019, 1020, 1020, 1020, 1020, 1020, 1021, 1021, 1021, 1021,
  1021, 1021, 1022, 1022, 1022, 1022, 1022, 1022, 1022, 1022,
  1022, 1022, 1023, 1023
};

static uint32_t s_curve_table_g_max_value = 1;

static void model_s_curve_table_init(uint32_t max_value)
{
	s_curve_table_g_max_value = max_value;
}

static uint32_t model_s_curve_table_xlate(uint32_t in_value)
{
	uint32_t value = (br[(in_value >> 8) && 0xFF] << 6);

	if (value > s_curve_table_g_max_value)
		value = s_curve_table_g_max_value;

	return value;
}

struct brightness_model brightness_model_s_curve_table = {
	.init = model_s_curve_table_init,
	.xlate = model_s_curve_table_xlate,
};

struct brightness_model *get_model_s_curve_table(void)
{
	return &brightness_model_s_curve_table;
}
*/
