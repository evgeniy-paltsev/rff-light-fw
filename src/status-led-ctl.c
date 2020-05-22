#include "status-led-ctl.h"

#include <drivers/gpio.h>
#include <sys/printk.h>

static void signal_led_worker(void);

static struct signal_led_ctx {
	/* private */
	struct device		*gpio_dev;

	/* public */
	u16_t			led_on_time;
	u16_t			led_off_time;
	u16_t			led_blink_count;
} signal_led;

/* minimum priority */
#define SIGNAL_LED_THREAD_PRIORITY	(CONFIG_NUM_PREEMPT_PRIORITIES - 1)
#define SIGNAL_LED_THREAD_STACKSIZE	400
#define SIGNAL_LED_PIN			DT_ALIAS_LED0_GPIOS_PIN

/* define thread with K_FOREVER, we will start it manually */
K_THREAD_DEFINE(led_worker_th, SIGNAL_LED_THREAD_STACKSIZE, signal_led_worker,
		NULL, NULL, NULL,
		SIGNAL_LED_THREAD_PRIORITY, 0, K_FOREVER);

void signal_led_init(void)
{
	signal_led.gpio_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	__ASSERT(signal_led.gpio_dev, "Signal LED GPIO device is NULL");

	gpio_pin_configure(signal_led.gpio_dev, SIGNAL_LED_PIN, GPIO_OUTPUT);

	/* LED off by default */
	gpio_pin_set(signal_led.gpio_dev, SIGNAL_LED_PIN, 1);

	signal_led.led_blink_count = 0;
	signal_led.led_on_time = 1000;
	signal_led.led_off_time = 1000;

	/* FIXME: according to code we can suspend thread before start,
	 * however it isn't documented */
	k_thread_suspend(led_worker_th);
	k_thread_start(led_worker_th);
}

void led_gpio_enable_if_not_blinking(bool on)
{
	if (signal_led.led_blink_count == 0)
		led_gpio_enable(on);
}

void led_gpio_enable(bool on)
{
	// TODO: do we need to check if thred is running
	/* suspend in case of led is already blinking? */
	k_thread_suspend(led_worker_th);

	//printk("RFF: led gpio thread: %s\n", k_thread_state_str(led_worker_th));

	if (on)
		gpio_pin_set(signal_led.gpio_dev, SIGNAL_LED_PIN, 0);
	else
		gpio_pin_set(signal_led.gpio_dev, SIGNAL_LED_PIN, 1);
}

void led_gpio_blink(u16_t on, u16_t off, u16_t count)
{
	// TODO: do we need to check if thred is running
	/* suspend in case of led is already blinking */
	k_thread_suspend(led_worker_th);

	signal_led.led_blink_count = count;
	signal_led.led_on_time = on;
	signal_led.led_off_time = off;

	k_thread_resume(led_worker_th);
}

static void signal_led_worker(void)
{
	while (true) {
		printk("RFF: got into led blink worker\n");

		while (signal_led.led_blink_count) {
			// printk("* led worker: count %u\n", signal_led.led_blink_count);
			gpio_pin_set(signal_led.gpio_dev, SIGNAL_LED_PIN, 0);
			k_sleep(signal_led.led_on_time);
			gpio_pin_set(signal_led.gpio_dev, SIGNAL_LED_PIN, 1);
			k_sleep(signal_led.led_off_time);

			signal_led.led_blink_count--;
		}

		k_thread_suspend(led_worker_th);
	}
}
