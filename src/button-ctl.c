#include "button-ctl.h"

#include <drivers/gpio.h>
#include <soc.h>
#include <sys/printk.h>

static struct device *button_gpio_dev;
static volatile enum button_state button_state = BUTTON_STATE_NOT_PRESSED;

static void button_init(void)
{
	button_gpio_dev = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	__ASSERT(button_gpio_dev, "BUTTON device is NULL");

	gpio_pin_configure(button_gpio_dev, DT_ALIAS_SW0_GPIOS_PIN,
			   GPIO_INPUT | GPIO_PULL_UP);
}

static inline bool button_pressed(void)
{
	return !gpio_pin_get(button_gpio_dev, DT_ALIAS_SW0_GPIOS_PIN);
}

static void button_ctl_worker(void)
{
	bool pressed;

	printk("RFF: got into button ctl worker\n");

	while (true) {
		pressed = button_pressed();

		if (pressed)
			button_state = BUTTON_STATE_PRESSED;
		else
			button_state = BUTTON_STATE_NOT_PRESSED;

		k_sleep(100);
	}
}

/* max priority */
#define BUTTON_CTL_THREAD_PRIORITY	(CONFIG_MAIN_THREAD_PRIORITY - 5)
//#define BUTTON_CTL_THREAD_PRIORITY	(CONFIG_NUM_PREEMPT_PRIORITIES - 5)
#define BUTTON_CTL_THREAD_STACKSIZE	400

/* define thread with K_FOREVER, we will start it manually */
K_THREAD_DEFINE(button_ctl_worker_th, BUTTON_CTL_THREAD_STACKSIZE, button_ctl_worker,
		NULL, NULL, NULL,
		BUTTON_CTL_THREAD_PRIORITY, 0, K_FOREVER);

void button_io_init(void)
{
	button_init();
	k_thread_start(button_ctl_worker_th);
}

enum button_state button_get_state(void)
{
	return button_state;
}
