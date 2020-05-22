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
#include "brightness-model/brightness-unified-model.h"
#include "button-ctl.h"
#include "status-led-ctl.h"
#include "bt/bt-hm11-ctl.h"
#include "alarm-params.h"
#include "core-model/core-model.h"
#include "dynamic-assert.h"


static struct host_cmd host_cmd_curr;

struct alarm_info {
	/* alarm thread related structures */
	struct k_thread alarm_worker_data;
	k_tid_t alarm_worker_tid;

	bool disarm_alarm;
};
K_MUTEX_DEFINE(disarm_mtx);

static struct alarm_info alarm_info;

#define ALARM_THREAD_PRIORITY		(CONFIG_NUM_PREEMPT_PRIORITIES - 1) //(CONFIG_NUM_PREEMPT_PRIORITIES - 3)
K_THREAD_STACK_DEFINE(alarm_worker_stack_area, 1024);
static void alarm_sched_worker(void *, void *, void *);


static struct core_model_params alarm_params = {
	.wait_time             = SLEEP_TIME_S - RISE_TIME_S,
	.rise_time             = RISE_TIME_S,
	.hold_max_time         = HOLD_TIME_S, /* 15 min */
	.decrease_to_mid_time  = 10,          /* 10 sec */
	.hold_mid_time         = 60 * 60,     /* 1 hour */
	.decrease_to_zero_time = 15,          /* 15 sec */
	.disarm_hold_mid_time  = 15 * 60,     /* 15 min */
};

static bool disarm_status_check_and_reset(void)
{
	bool disarm_status = false;

	k_mutex_lock(&disarm_mtx, K_FOREVER);
	if (alarm_info.disarm_alarm) {
		alarm_info.disarm_alarm = false;
		disarm_status = true;
	}
	k_mutex_unlock(&disarm_mtx);

	return disarm_status;
}

static void led_mark_xlation(enum brightnes_value_ops brightnes_from,
			     enum brightnes_value_ops brightnes_to)
{
	/* Enable led when we change brightnes */
	led_gpio_enable_if_not_blinking(brightnes_from != brightnes_to);
}

static void alarm_sched_worker(void *nu0, void *nu1, void *nu2)
{
	struct core_model_io io;
	uint32_t led0, led1;

	printk("RFF: got into alarm worker\n");

	alarm_params.wait_time = rtc_get_alarm();
	io.disarm_info = atomic_state_get();
	io.disarm_time_adjustment = backup_data_get();

	do {
		// printk("* alarm worker\n");

		if (disarm_status_check_and_reset())
			io.disarm_info |= A_DISARM_NEW;

		do_alarm_model(rtc_get_time(), &io, &alarm_params);
		backup_data_set(io.disarm_time_adjustment);
		atomic_state_set(io.disarm_info);
		led_mark_xlation(io.brightnes_from, io.brightnes_to);
		brightness_log_xlate_to_2_auto(io.brightnes_from,
					       io.brightnes_to,
					       io.brightnes_period,
					       io.brightnes_curr_value,
					       &led0, &led1);

		TIM3->CCR4 = led0;
		TIM3->CCR3 = led1;

		k_sleep(100);

	} while (!io.finished);

	printk("RFF: alarm finished at %u\n", rtc_get_time());
}

static void disarm_alarm_force(void)
{
	k_mutex_lock(&disarm_mtx, K_FOREVER);
	alarm_info.disarm_alarm = true;
	k_mutex_unlock(&disarm_mtx);
}

static void alarm_init_new(uint32_t sleep_time)
{
	printk("RFF: schedule new alarm for: %uh %um %us\n",
			(sleep_time / (60 * 60)),
			(sleep_time % (60 * 60)) / 60,
			(sleep_time % (60 * 60)) % 60);

	/* other params left unchanged */
	alarm_params.wait_time = sleep_time - RISE_TIME_S;
	backup_data_set(0);
	atomic_state_set(A_DISARM_NONE);
	rtc_set_alarm(alarm_params.wait_time);
}

static void alarm_powerfault_restore(void)
{
	printk("RFF: repair alarm after power fault\n");

	dynamic_assert(!alarm_info.alarm_worker_tid);

	k_mutex_init(&disarm_mtx);
	alarm_info.disarm_alarm = false;

	alarm_info.alarm_worker_tid = k_thread_create(&alarm_info.alarm_worker_data,
					alarm_worker_stack_area,
					K_THREAD_STACK_SIZEOF(alarm_worker_stack_area),
					alarm_sched_worker,
					NULL, NULL, NULL,
					ALARM_THREAD_PRIORITY, 0, K_NO_WAIT);
}

static void alarm_init_new_new(uint32_t sleep_time)
{
	printk("RFF: start new alarm\n");

	dynamic_assert(sleep_time >= RISE_TIME_S);

	//TODO: use manual temination instead of abort
	if (alarm_info.alarm_worker_tid) {
		printk("RFF: drop previous alarm worker as we are starting new one\n");
		k_thread_abort(alarm_info.alarm_worker_tid);
	}

	led_gpio_blink(20, 20, 250);

	k_mutex_init(&disarm_mtx);
	alarm_info.disarm_alarm = false;

	alarm_init_new(sleep_time);
	alarm_info.alarm_worker_tid = k_thread_create(&alarm_info.alarm_worker_data,
					alarm_worker_stack_area,
					K_THREAD_STACK_SIZEOF(alarm_worker_stack_area),
					alarm_sched_worker,
					NULL, NULL, NULL,
					ALARM_THREAD_PRIORITY, 0, K_NO_WAIT);
}


/* After POR if button is pressed - user wants new alarm with default values */
static void check_for_alarm(void)
{
	if (button_get_state() == BUTTON_STATE_PRESSED)
		alarm_init_new_new(SLEEP_TIME_S);
	else
		alarm_powerfault_restore();
}

static void send_alarm_info(void)
{
	if (true /* FIXME: check for alarm exists */) {
		char str[RESPOND_BUFF_SZ];
		uint32_t remind_time = 0;
		uint32_t sleep_time = alarm_params.wait_time + alarm_params.rise_time;

		if (sleep_time > rtc_get_time())
			remind_time = sleep_time - rtc_get_time();

		snprintf(str, RESPOND_BUFF_SZ, "REM %2d:%2d:%2d",
			 (remind_time / (60 * 60)),
			 ((remind_time % (60 * 60)) / 60),
			 ((remind_time % (60 * 60)) % 60));

		hm11_send_cmd_respond(str);

		/* hacks for nRF Connect users :) */
		k_sleep(2500);

		snprintf(str, RESPOND_BUFF_SZ, "SLEEP %2d:%2d",
			 (sleep_time / (60 * 60)),
			 ((sleep_time % (60 * 60)) / 60));

		hm11_send_cmd_respond(str);
	} else {
		hm11_send_cmd_respond("NO ALARM");
	}
}

void main(void)
{
	int ret;

	printk("RFF: Hello World! %s\n", CONFIG_BOARD);

	printk("Default alarm_params: default SLEEP_TIME_S %u:%u:%u, RISE_TIME %u m, HOLD_TIME %u m\n",
			SLEEP_TIME_S / (60 * 60), (SLEEP_TIME_S % (60 * 60)) / 60,
			(SLEEP_TIME_S % (60 * 60)) % 60,
			RISE_TIME_S / 60, HOLD_TIME_S / 60);

	rtc_init();
	button_io_init(true);
	button_print_debug();
	bt_uart_init();
	signal_led_init();
	timer3_pwm_init();

	check_for_alarm();

	hm11_init();

	while (1) {
		ret = hm11_wait_for_host_cmd(&host_cmd_curr);
		if (ret)
			continue;

		if (host_cmd_curr.type == HOST_CMD_SCHEDULE_ALARM)
			alarm_init_new_new(host_cmd_curr.delay_sec);
		else if (host_cmd_curr.type == HOST_CMD_DISARM_ALARM)
			disarm_alarm_force();
		else if (host_cmd_curr.type == HOST_CMD_COMMON_INFO)
			send_alarm_info();
	}
}
