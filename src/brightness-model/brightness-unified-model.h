#ifndef BRIGHTNESS_UNIFIED_MODEL_H_
#define BRIGHTNESS_UNIFIED_MODEL_H_

#include <stdint.h>

#include "../brightness-ops.h"

/* xlate to 2 fixed outputs */
void brightness_log_xlate_to_2(enum brightnes_value_ops brightnes_from,
			       enum brightnes_value_ops brightnes_to,
			       enum brightnes_geometry_ops geometry,
			       uint32_t brightnes_period,
			       uint32_t brightnes_curr_value,
			       uint32_t *led0, uint32_t *led1);



void brightness_log_xlate_to_2_auto(enum brightnes_value_ops brightnes_from,
				    enum brightnes_value_ops brightnes_to,
				    uint32_t brightnes_period,
				    uint32_t xlate_value,
				    uint32_t *led0, uint32_t *led1);

#endif
