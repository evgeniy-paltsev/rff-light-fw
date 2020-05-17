#ifndef BUTTON_CTL_H_
#define BUTTON_CTL_H_

#include <soc.h>

enum button_state {
	BUTTON_STATE_NOT_PRESSED = 0,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_PRESSED_LONG
};

void button_io_init(bool sync);
enum button_state button_get_state(void);
void button_print_debug(void);

#endif
