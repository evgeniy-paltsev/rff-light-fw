#include "button-ctl.h"

#include <drivers/gpio.h>
#include <sys/printk.h>

/* ULL defines are stil missing in zephyr 2.2.0 */
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _ULL(x)		(_AC(x, ULL))
#define ULL(x)		(_ULL(x))
#define BITS_PER_LONG_LONG 64
#define GENMASK_ULL(h, l) \
	(((~ULL(0)) - (ULL(1) << (l)) + 1) & \
	 (~ULL(0) >> (BITS_PER_LONG_LONG - 1 - (h))))

#define BUTTON_READS_STD		7
/* about 0.2 sec (50) at least */
#define BUTTON_READS_STD_TRASHOLD	4
#define BUTTON_READS_LONG		63
/* about 2.85 sec (50) at least */
#define BUTTON_READS_LONG_TRASHOLD	57

#define READ_PERIOD			25

static struct device *button_gpio_dev;
static volatile enum button_state button_state = BUTTON_STATE_NOT_PRESSED;
static u64_t reads_raw;

static void button_init(void)
{
	button_gpio_dev = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	__ASSERT(button_gpio_dev, "BUTTON device is NULL");

	gpio_pin_configure(button_gpio_dev, DT_ALIAS_SW0_GPIOS_PIN,
			   GPIO_INPUT | GPIO_PULL_UP);
}

static inline bool button_raw_read(void)
{
	return !gpio_pin_get(button_gpio_dev, DT_ALIAS_SW0_GPIOS_PIN);
}

static inline void button_raw_fill(void)
{
	bool pressed = button_raw_read();

	reads_raw <<= 1;
	reads_raw |= pressed;
}

static inline void button_raw_process(void)
{
	enum button_state state;

	if (__builtin_popcountll(reads_raw & GENMASK_ULL(BUTTON_READS_LONG, 0)) >= BUTTON_READS_LONG_TRASHOLD)
		state = BUTTON_STATE_PRESSED_LONG;
	else if (__builtin_popcountll(reads_raw & GENMASK_ULL(BUTTON_READS_STD, 0)) >= BUTTON_READS_STD_TRASHOLD)
		state = BUTTON_STATE_PRESSED;
	else
		state = BUTTON_STATE_NOT_PRESSED;

	button_state = state;
}

static void button_ctl_worker(void)
{
	printk("RFF: got into button ctl worker\n");

	while (true) {
		// printk("* button worker\n");
		button_raw_fill();
		button_raw_process();

		k_sleep(READ_PERIOD);
	}
}

/* read button enough times to determine state: PRESSED / NOT_PRESSED. We don't
 * support PRESSED_LONG here. */
static void fill_button_read_pool(void)
{
	for (int i = 0; i < BUTTON_READS_STD; i++) {
		button_raw_fill();
		k_sleep(READ_PERIOD);
	}

	button_raw_process();
}

/* max priority */
#define BUTTON_CTL_THREAD_PRIORITY	(CONFIG_MAIN_THREAD_PRIORITY - 5)
//#define BUTTON_CTL_THREAD_PRIORITY	(CONFIG_NUM_PREEMPT_PRIORITIES - 5)
#define BUTTON_CTL_THREAD_STACKSIZE	400

/* define thread with K_FOREVER, we will start it manually */
K_THREAD_DEFINE(button_ctl_worker_th, BUTTON_CTL_THREAD_STACKSIZE, button_ctl_worker,
		NULL, NULL, NULL,
		BUTTON_CTL_THREAD_PRIORITY, 0, K_FOREVER);

void button_io_init(bool sync)
{
	button_init();
	if (sync)
		fill_button_read_pool();
	k_thread_start(button_ctl_worker_th);
}

enum button_state button_get_state(void)
{
	return button_state;
}

void button_print_debug(void)
{
	u64_t _reads_raw = reads_raw;
	enum button_state state = button_state;

	printk("RFF: button: raw %16llx (%2u of %u, %2u of %u) state %s \n",
		_reads_raw,
		__builtin_popcountll(_reads_raw & GENMASK_ULL(BUTTON_READS_LONG, 0)),
		BUTTON_READS_LONG_TRASHOLD,
		__builtin_popcountll(_reads_raw & GENMASK_ULL(BUTTON_READS_STD, 0)),
		BUTTON_READS_STD_TRASHOLD,
		(state == BUTTON_STATE_PRESSED_LONG) ? "pressed long" :
		(state == BUTTON_STATE_PRESSED) ? "pressed short" : "not pressed");
}
