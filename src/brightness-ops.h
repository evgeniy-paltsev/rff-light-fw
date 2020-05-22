#ifndef BRIGHTNESS_OPS_H_
#define BRIGHTNESS_OPS_H_

enum brightnes_value_ops {
	BRIGHTNESS_OFF = 0,
	BRIGHTNESS_MID,
	BRIGHTNESS_MAX,
	/* Use current brightnes from HW or BRIGHTNESS_OFF in case this value is missing
	 * Used only for user interructions - disarming */
	BRIGHTNESS_THIS_OR_OFF
};

enum brightnes_geometry_ops {
	B_LED_SEQUENTIAL = 0,
	B_LED_PARALLEL
};

#endif
