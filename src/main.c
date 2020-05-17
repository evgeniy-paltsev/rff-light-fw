#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pwm/tim3-pwm.h"
#include "rtc/rtc-ctl.h"
#include "brightness-model/brightness-model.h"
#include "button-ctl.h"
#include "status-led-ctl.h"
#include "bt/bt-hm11-ctl.h"
#include "alarm-params.h"

#define runtime_assert(cond_true) ({											\
	if (!(cond_true)) {												\
		while (true) {												\
			printk(" ! assertion failed, %s %d, %u\n", __func__, __LINE__, rtc_get_time());			\
		}													\
	}														\
})


static struct brightness_model brightness_model;
static struct host_cmd host_cmd_curr;

enum alarm_state {
	ALARM_STATE_NONE = 0,
	ALARM_STATE_SCHEDULED,
	ALARM_STATE_FIRED_RISE,
	ALARM_STATE_FIRED_HOLD,
	ALARM_STATE_DONE,
	ALARM_STATE_DISARMED,
};

static inline void alarm_atomic_state_set(enum alarm_state state)
{
	atomic_state_set(state);
}

static inline enum alarm_state alarm_atomic_state_get(void)
{
	return (enum alarm_state)(atomic_state_get());
}

static void alarm_start(uint32_t sleep_time)
{
	printk("RFF: schedule new alarm for: %uh %um %us\n",
			(sleep_time / (60 * 60)),
			(sleep_time % (60 * 60)) / 60,
			(sleep_time % (60 * 60)) % 60);

	backup_data_set(0);
	rtc_set_alarm(sleep_time);
	alarm_atomic_state_set(ALARM_STATE_SCHEDULED);

	TIM3->CCR4 = 0;
	TIM3->CCR3 = 0;
}

struct alarm_info {
	/* alarm thread related structures */
	struct k_thread alarm_worker_data;
	k_tid_t alarm_worker_tid;

	bool powerfault_happened;
	bool disarm_alarm;
};

static struct alarm_info alarm_info;

K_THREAD_STACK_DEFINE(alarm_worker_stack_area, 1024);

#define ALARM_THREAD_PRIORITY		(CONFIG_NUM_PREEMPT_PRIORITIES - 3)
static void alarm_sched_worker(void *, void *, void *);

static void schedule_alarm_new(uint32_t wait_time)
{
	//TODO: use manual temination instead of abort
	if (alarm_info.alarm_worker_tid) {
		printk("RFF: drop previous alarm worker\n");
		k_thread_abort(alarm_info.alarm_worker_tid);
	}

	runtime_assert(wait_time >= RISE_TIME_S);

	model_logarithmic_init(&brightness_model, RISE_TIME_S);

	alarm_info.powerfault_happened = false;
	alarm_info.disarm_alarm = false;

	alarm_start(wait_time - RISE_TIME_S);
	alarm_info.alarm_worker_tid = k_thread_create(&alarm_info.alarm_worker_data,
					alarm_worker_stack_area,
					K_THREAD_STACK_SIZEOF(alarm_worker_stack_area),
					alarm_sched_worker,
					NULL, NULL, NULL,
					ALARM_THREAD_PRIORITY, 0, K_NO_WAIT);
}

static void check_for_alarm_powerfault(void)
{
	if (alarm_atomic_state_get() != ALARM_STATE_SCHEDULED  &&
	    alarm_atomic_state_get() != ALARM_STATE_FIRED_RISE &&
	    alarm_atomic_state_get() != ALARM_STATE_FIRED_HOLD)
		return;

	runtime_assert(!alarm_info.alarm_worker_tid);

	printk("RFF: repair alarm after power fault\n");

	model_logarithmic_init(&brightness_model, RISE_TIME_S);

	alarm_info.powerfault_happened = true;
	alarm_info.disarm_alarm = false;

	alarm_info.alarm_worker_tid = k_thread_create(&alarm_info.alarm_worker_data,
					alarm_worker_stack_area,
					K_THREAD_STACK_SIZEOF(alarm_worker_stack_area),
					alarm_sched_worker,
					NULL, NULL, NULL,
					ALARM_THREAD_PRIORITY, 0, K_NO_WAIT);
}

/* After reset button is pressed - user wants new alarm with default values */
static void sheck_for_alarm(void)
{
	if (button_get_state() == BUTTON_STATE_PRESSED)
		schedule_alarm_new(SLEEP_TIME);
	else
		check_for_alarm_powerfault();
}

static void xlate_leds(uint32_t val)
{
	uint32_t led0, led1;

	brightness_model.xlate_leds(&brightness_model, val, &led0, &led1);
	TIM3->CCR4 = led0;
	TIM3->CCR3 = led1;
}

static void _alarm_sched_worker(u32_t *suspend_time)
{
	uint32_t curr_time = 0, start_time = 0, new_time;

	printk("RFF: got into led alarm worker\n");
	led_gpio_blink(20, 20, 250);

	/* default safe value for led slow disabling */
	*suspend_time = RISE_TIME_S;

	runtime_assert(alarm_atomic_state_get() == ALARM_STATE_SCHEDULED  ||
		       alarm_atomic_state_get() == ALARM_STATE_FIRED_RISE ||
		       alarm_atomic_state_get() == ALARM_STATE_FIRED_HOLD);

	while (!rtc_is_alarm())
		k_sleep(200);

	printk("RFF: rise period start = %u\n", rtc_get_time());
	alarm_atomic_state_set(ALARM_STATE_FIRED_RISE);

	start_time = rtc_get_alarm();
	curr_time = rtc_get_time();

	runtime_assert(RISE_TIME_S + start_time < 0xFFFF);

	while (start_time + RISE_TIME_S > curr_time) {
		new_time = rtc_get_time();

		if (new_time != curr_time) {
			curr_time = new_time;
			xlate_leds(curr_time - start_time);
		}

		/* save timestamp to calculate brightens for decrease from it later */
		backup_data_set(curr_time - start_time);

		if (button_get_state() == BUTTON_STATE_PRESSED) {
			*suspend_time = new_time;
			printk("RFF: alarm suspended at = %u\n", rtc_get_time());
			alarm_atomic_state_set(ALARM_STATE_DONE);

			return;
		}

		k_sleep(100);
	}

	printk("RFF: rise period end = %u\n", rtc_get_time());
	alarm_atomic_state_set(ALARM_STATE_FIRED_HOLD);

	start_time += RISE_TIME_S;
	curr_time = rtc_get_time();

	/* TODO: we don't handle power fault for HOLD time */
	if (start_time + HOLD_TIME_S > curr_time) {
		printk("RFF: adjust brightens for holding = %u\n", rtc_get_time());
		xlate_leds(RISE_TIME_S + 1);
	}

	while (start_time + HOLD_TIME_S > curr_time) {
		curr_time = rtc_get_time();

		if (button_get_state() == BUTTON_STATE_PRESSED) {
			printk("RFF: alarm suspended = %u\n", rtc_get_time());
			alarm_atomic_state_set(ALARM_STATE_DONE);

			return;
		}

		k_sleep(100);
	}

	printk("RFF: hold period end = %u\n", rtc_get_time());
	alarm_atomic_state_set(ALARM_STATE_DONE);
}

#define END_ALARM_BRIGHTNES	0xDFE
#define SLOW_DOWN_TIME_S	15
#define MAX_PWM 0xFFFF

/* slowly decrease brightens to safe value */
static void suspend_alarm(u32_t suspend_time)
{
//	uint32_t curr_time;
//	uint32_t curr_brightness_timestamp;

	runtime_assert(alarm_atomic_state_get() == ALARM_STATE_DONE);

	printk("RFF: decrease brightens to safe value = %u\n", rtc_get_time());

//	curr_brightness_timestamp = backup_data_get();
//	curr_time = rtc_get_time();
//
//	brightness_model->xlate_leds(curr_brightness_timestamp, &led0, &led1);
//	TIM3->CCR4 = led0;
//	TIM3->CCR3 = led1;
//
//	while (true) {
//		if (curr_time != rtc_get_time()) {
//			curr_brightness_timestamp -= RISE_TIME_S / SLOW_DOWN_TIME_S;
//			backup_data_set(curr_brightness_timestamp);
//			curr_time = rtc_get_time();
//
//			brightness_model->xlate_leds(curr_brightness_timestamp, &led0, &led1);
//			TIM3->CCR4 = led0;
//			TIM3->CCR3 = led1;
//
//			if (led0 < END_ALARM_BRIGHTNES)
//				break;
//		}
//
//		k_sleep(100);
//	}
//	printk("RFF: backup data is = %u, = %u\n", backup_data_get(), rtc_get_time());

	/*
	 * In normal case this brightens adjustment is useless.
	 * It's only useful in case of calling this function after power fault.
	 */
	xlate_leds(backup_data_get());

//	if (TIM3->CCR4 < END_ALARM_BRIGHTNES)
//		TIM3->CCR4 = END_ALARM_BRIGHTNES;
//
//	if (TIM3->CCR3 < END_ALARM_BRIGHTNES)
//		TIM3->CCR3 = END_ALARM_BRIGHTNES;

	led_gpio_blink(500, 800, 100);
	for (uint32_t i = 0; i < MAX_PWM / 300; i++) {
		u32_t led0 = TIM3->CCR4;
		u32_t led1 = TIM3->CCR3;

		if (led0 > END_ALARM_BRIGHTNES)
			led0 -= 300;

		if (led1 > END_ALARM_BRIGHTNES)
			led1 -= 300;

		TIM3->CCR4 = led0;
		TIM3->CCR3 = led1;

		k_sleep(50);
	}
	led_gpio_enable(false);


	printk("RFF: brightens at safe value, wait for user = %u\n", rtc_get_time());

	/* TODO: add timeout here, like 1-2 hours */
	while (!(button_get_state() == BUTTON_STATE_PRESSED))
		k_sleep(100);

	printk("RFF: alarm fully disabled, bye = %u\n", rtc_get_time());

	TIM3->CCR4 = 0;
	TIM3->CCR3 = 0;
}

static void alarm_sched_worker(void *nu0, void *nu1, void *nu2)
{
	u32_t suspend_time = RISE_TIME_S;

	_alarm_sched_worker(&suspend_time);
	suspend_alarm(suspend_time);
}

static void force_disarm_alarm()
{
	alarm_info.disarm_alarm = true;
//
//	/* if we have alarm not fred - simply abort alarm thread */
//	if (atomic_state_get() == ALARM_STATE_SCHEDULED) {
//		if (alarm_info.alarm_worker_tid) {
//			printk("RFF: drop alarm worker\n");
//			k_thread_abort(alarm_info.alarm_worker_tid);
//		}
//	}
}

void main(void)
{
	int ret;

	printk("RFF: Hello World! %s\n", CONFIG_BOARD);

	rtc_init();
	button_io_init();
	printk("RFF: button_pressed %d\n", button_get_state() == BUTTON_STATE_PRESSED);
	bt_uart_init();
	signal_led_init();
	timer3_pwm_init();

	model_logarithmic_init(&brightness_model, RISE_TIME_S);

	sheck_for_alarm();

	hm11_do_at_cmd(hm11_assert);

	printk("Defined params: default SLEEP_TIME %u:%u:%u, RISE_TIME %u m, HOLD_TIME %u m\n",
			SLEEP_TIME / (60 * 60), (SLEEP_TIME % (60 * 60)) / 60,
			(SLEEP_TIME % (60 * 60)) % 60,
			RISE_TIME_S / 60, HOLD_TIME_S / 60);

	while (1) {
		ret = hm11_wait_for_host_cmd(&host_cmd_curr);
		if (ret)
			continue;

		if (host_cmd_curr.type == HOST_CMD_SCHEDULE_ALARM)
			schedule_alarm_new(host_cmd_curr.delay_sec);
		else if (host_cmd_curr.type == HOST_CMD_DISARM_ALARM)
			force_disarm_alarm();
	}
}
