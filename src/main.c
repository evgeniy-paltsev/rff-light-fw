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
	.hold_max_time         = HOLD_MAXT_S, /* 15 min */
	.decrease_to_mid_time  = 10,          /* 10 sec */
	.hold_mid_time         = HOLD_MIDT_S, /* 1 hour */
	.decrease_to_zero_time = 25,          /* 25 sec */
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

#define ALARM_SCHED_BACKOFF_MS		100

static void rtc_watchdog(bool init, uint32_t curr_rtc)
{
	static uint32_t prev_rtc = 0, same_time_counter = 0, same_time_counter_max = 0;
	static bool triggered = false;

	uint32_t trashold = 1000 / ALARM_SCHED_BACKOFF_MS + 2;

	if (init) {
		prev_rtc = curr_rtc;
		triggered = false;
		same_time_counter = 0;
		same_time_counter_max = 0;

		return;
	}

	/* don't care about uint32_t overflow here */
	if (prev_rtc == curr_rtc)
		same_time_counter++;
	else
		same_time_counter = 0;

	prev_rtc = curr_rtc;

	if (same_time_counter >= trashold)
		triggered = true;

	if (same_time_counter > same_time_counter_max)
		same_time_counter_max = same_time_counter;

	if (triggered)
		printk("RFF: detect rtc hang for max %u times (trashold = %u)\n",
		       same_time_counter_max, trashold);
}



static void alarm_sched_worker(void *nu0, void *nu1, void *nu2)
{
	struct core_model_io io;
	uint32_t led0, led1;

	printk("RFF: got into alarm worker, curr time %u\n", rtc_get_time());

	alarm_params.wait_time = rtc_get_alarm();
	io.atomic_alarm_info = atomic_state_get();
	io.disarm_time_adjustment = backup_data_get();

	rtc_watchdog(true, rtc_get_time());

	do {
		if (disarm_status_check_and_reset())
			set_disarm_new(&io);

		rtc_watchdog(false, rtc_get_time());
		do_alarm_model(rtc_get_time(), &io, &alarm_params);
		backup_data_set(io.disarm_time_adjustment);
		atomic_state_set(io.atomic_alarm_info);
		led_mark_xlation(io.brightnes_from, io.brightnes_to);
		brightness_log_xlate_to_2_auto(io.brightnes_from,
					       io.brightnes_to,
					       io.brightnes_period,
					       io.brightnes_curr_value,
					       &led0, &led1);

		TIM3->CCR4 = led0;
		TIM3->CCR3 = led1;
		// printk("RFF: set leds 0x%04x : 0x%04x\n", led0, led1);

		k_sleep(ALARM_SCHED_BACKOFF_MS);

	} while (!alarm_finished(&io));

	printk("RFF: alarm finished at %u\n", rtc_get_time());
}

static inline void disarm_status_set(void)
{
	k_mutex_lock(&disarm_mtx, K_FOREVER);
	alarm_info.disarm_alarm = true;
	k_mutex_unlock(&disarm_mtx);
}

static void disarm_alarm_bt(void)
{
	printk("RFF: disarm alarm via BT\n");
	disarm_status_set();
}

static void disarm_alarm_button(void)
{
	printk("RFF: disarm alarm by button\n");
	disarm_status_set();
}

static void alarm_init_new(uint32_t sleep_time)
{
	printk("RFF: schedule new alarm for: %uh %um %us (%us)\n",
			(sleep_time / (60 * 60)),
			(sleep_time % (60 * 60)) / 60,
			(sleep_time % (60 * 60)) % 60,
			(sleep_time));

	/* other params left unchanged */
	alarm_params.wait_time = sleep_time - RISE_TIME_S;
	printk("RFF: trigger wait time: %uh %um %us (%us)\n",
			(alarm_params.wait_time / (60 * 60)),
			(alarm_params.wait_time % (60 * 60)) / 60,
			(alarm_params.wait_time % (60 * 60)) % 60,
			(alarm_params.wait_time));

	backup_data_set(0);
	atomic_state_set(A_ALARM_NEW);
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
	printk("RFF: start new alarm for sleep time %us\n", sleep_time);

	dynamic_assert(sleep_time >= RISE_TIME_S);

	//TODO: use manual temination instead of abort
	if (alarm_info.alarm_worker_tid) {
		printk("RFF: drop previous alarm worker as we are starting new one\n");
		k_thread_abort(alarm_info.alarm_worker_tid);
	}

	led_gpio_blink(20, 30, 250);

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
	else if (!(atomic_state_get() & A_ALARM_FINISHED))
		alarm_powerfault_restore();
}

static void send_alarm_info_hank(const char *respond)
{
	printk("RFF: alarm_info: %s\n", respond);
	hm11_send_cmd_respond(respond);
}

static void send_alarm_info(void)
{
	if (atomic_state_get() & A_ALARM_FINISHED) {
		send_alarm_info_hank("NO ALARM");
	} else {
		char str[RESPOND_BUFF_SZ];
		uint32_t curr_time, remind_time = 0;
		uint32_t sleep_time = alarm_params.wait_time + alarm_params.rise_time;

		curr_time = rtc_get_time();

		if (sleep_time > curr_time)
			remind_time = sleep_time - curr_time;
		else
			remind_time = 0;

		/* time remains before wake up */
		snprintf(str, RESPOND_BUFF_SZ, "REM %u:%u:%u",
			 (remind_time / (60 * 60)),
			 ((remind_time % (60 * 60)) / 60),
			 ((remind_time % (60 * 60)) % 60));

		send_alarm_info_hank(str);
		/* hacks for nRF Connect users :) */
		k_sleep(3000);

		if (alarm_params.wait_time > curr_time)
			remind_time = alarm_params.wait_time - curr_time;
		else
			remind_time = 0;

		/* time remains before rise */
		snprintf(str, RESPOND_BUFF_SZ, "wREM %u:%u:%u",
			 (remind_time / (60 * 60)),
			 ((remind_time % (60 * 60)) / 60),
			 ((remind_time % (60 * 60)) % 60));

		send_alarm_info_hank(str);
		/* hacks for nRF Connect users :) */
		k_sleep(3000);

		snprintf(str, RESPOND_BUFF_SZ, "SLEEP %u:%u",
			 (sleep_time / (60 * 60)),
			 ((sleep_time % (60 * 60)) / 60));

		send_alarm_info_hank(str);
		/* hacks for nRF Connect users :) */
		k_sleep(3000);

		snprintf(str, RESPOND_BUFF_SZ, "CURRt %u:%u:%u",
			 (curr_time / (60 * 60)),
			 ((curr_time % (60 * 60)) / 60),
			 ((curr_time % (60 * 60)) % 60));

		send_alarm_info_hank(str);
	}
}

void main(void)
{
	int ret;

	printk("RFF: Hello World! %s\n", CONFIG_BOARD);

	printk("%s alarm params: default SLEEP_TIME_S %u:%u:%u, RISE_TIME %u m, HOLD_TIME %u m\n",
			IS_ENABLED(CONFIG_ALARM_TESTING_SESSION) ? "  ** test launch **  " : "RFF:",
			SLEEP_TIME_S / (60 * 60), (SLEEP_TIME_S % (60 * 60)) / 60,
			(SLEEP_TIME_S % (60 * 60)) % 60,
			RISE_TIME_S / 60, HOLD_MAXT_S / 60);

	rtc_init(A_ALARM_POR_VALUE);
	button_io_init(true, disarm_alarm_button);
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
			alarm_init_new_new(host_cmd_curr.cmd_u32_param_0);
		else if (host_cmd_curr.type == HOST_CMD_DISARM_ALARM)
			disarm_alarm_bt();
		else if (host_cmd_curr.type == HOST_CMD_COMMON_INFO)
			send_alarm_info();
		/* Other command (HOST_CMD_PING) doesn't requre special handling
		 * and fully processed in hm11_wait_for_host_cmd */
	}
}
