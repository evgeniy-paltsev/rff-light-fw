#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pwm/tim3-pwm.h"
#include "rtc/rtc-ctl.h"
#include "brightness-model/brightness-model.h"

#define str(a) #a
#define xstr(a) str(a)

#define RISE_TIME_S		(20 * 60)//60/
#define HOLD_TIME_S		(15 * 60)//60/

#define SLEEP_TIME_MIN		30
#define SLEEP_TIME_HOURS	7

#define SLEEP_TIME		((SLEEP_TIME_HOURS * 60 + SLEEP_TIME_MIN) * 60)

#define runtime_assert(cond_true) ({											\
	if (!(cond_true)) {												\
		while (true) {												\
			printk(" ! assertion failed, %s %d, %u\n", __func__, __LINE__, rtc_get_time());			\
		}													\
	}														\
})


enum host_cmd_type {
	HOST_CMD_NONE = 0,
	HOST_CMD_SCHEDULE_ALARM,
	HOST_CMD_DISARM_ALARM,
	HOST_CMD_COMMON_INFO,
};

static struct host_cmd {
	enum host_cmd_type	type;
	uint32_t		delay_sec;
} host_cmd_curr;

static struct brightness_model *brightness_model;

static void signal_led_worker(void);

static struct signal_led_ctx {
	/* private */
	struct device		*gpio_dev;

	/* public */
	uint16_t		led_on_time;
	uint16_t		led_off_time;
	uint16_t		led_blink_count;
} signal_led;

/* minimum priority */
#define SIGNAL_LED_THREAD_PRIORITY	(CONFIG_NUM_PREEMPT_PRIORITIES - 2)
#define SIGNAL_LED_THREAD_STACKSIZE	400
#define SIGNAL_LED_PIN			DT_ALIAS_LED0_GPIOS_PIN

/* define thread with K_FOREVER, we will start it manually */
K_THREAD_DEFINE(led_worker_th, SIGNAL_LED_THREAD_STACKSIZE, signal_led_worker,
		NULL, NULL, NULL,
		SIGNAL_LED_THREAD_PRIORITY, 0, K_FOREVER);

static void signal_led_init(void)
{
	signal_led.gpio_dev = device_get_binding(DT_ALIAS_LED0_GPIOS_CONTROLLER);
	__ASSERT(signal_led.gpio_dev, "Signal LED GPIO device is NULL");

	gpio_pin_configure(signal_led.gpio_dev, SIGNAL_LED_PIN, GPIO_DIR_OUT);

	/* LED off by default */
	gpio_pin_write(signal_led.gpio_dev, SIGNAL_LED_PIN, 1);

	signal_led.led_blink_count = 0;
	signal_led.led_on_time = 1000;
	signal_led.led_off_time = 1000;

	/* FIXME: according to code we can suspend thread before start,
	 * however it isn't documented */
	k_thread_suspend(led_worker_th);
	k_thread_start(led_worker_th);
}

static void led_gpio_enable(bool on)
{
	// TODO: do we need to check if thred is running
	/* suspend in case of led is already blinking? */
	k_thread_suspend(led_worker_th);

	printk("RFF: led gpio thread: %s\n", k_thread_state_str(led_worker_th));

	if (on)
		gpio_pin_write(signal_led.gpio_dev, SIGNAL_LED_PIN, 0);
	else
		gpio_pin_write(signal_led.gpio_dev, SIGNAL_LED_PIN, 1);
}

static __unused void led_gpio_blink(uint16_t on, uint16_t off, uint16_t count)
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

		while (signal_led.led_blink_count--) {
			gpio_pin_write(signal_led.gpio_dev, SIGNAL_LED_PIN, 0);
			k_sleep(signal_led.led_on_time);
			gpio_pin_write(signal_led.gpio_dev, SIGNAL_LED_PIN, 1);
			k_sleep(signal_led.led_off_time);
		}

		k_thread_suspend(led_worker_th);
	}
}

static struct device *button_gpio_dev;

static void button_init(void)
{
	button_gpio_dev = device_get_binding(DT_ALIAS_SW0_GPIOS_CONTROLLER);
	__ASSERT(button_gpio_dev, "BUTTON device is NULL");

	gpio_pin_configure(button_gpio_dev, DT_ALIAS_SW0_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_PUD_PULL_UP);
}

static inline bool button_pressed(void)
{
	u32_t val = 0U;

	gpio_pin_read(button_gpio_dev, DT_ALIAS_SW0_GPIOS_PIN, &val);
	return !val;
}

static void bt_uart_isr(struct device *uart_dev);

#define MAX_RX_SIZE	64
#define MAX_NOTIF_SIZE	2

struct ser_ctx {
	struct device	*uart_dev;

	stack_data_t	stack_arr[MAX_NOTIF_SIZE];
	struct k_stack	notify_stack;

	u8_t		rx_buff[MAX_RX_SIZE + 2];
	/* can be reseted only with IRQ disabled */
	u8_t		data_recieved;

	/* can be changed only with IRQ disabled */
	u8_t		data_notify_second;
} ser;

static void bt_uart_init(void)
{
	ser.uart_dev = device_get_binding(DT_UART_STM32_USART_2_NAME);
	__ASSERT(ser.uart_dev, "UART device is NULL");

	uart_irq_rx_disable(ser.uart_dev);
	uart_irq_tx_disable(ser.uart_dev);

	k_stack_init(&ser.notify_stack, ser.stack_arr, MAX_NOTIF_SIZE);

	uart_irq_callback_set(ser.uart_dev, bt_uart_isr);
}

static void bt_uart_reconfig(u8_t data_receive_expected)
{
	u8_t c;

	uart_irq_rx_disable(ser.uart_dev);

	__ASSERT(ser.uart_dev, "UART device is NULL");
	__ASSERT(data_receive_expected <= MAX_RX_SIZE, "data_receive_expected too much");

	if (data_receive_expected > MAX_RX_SIZE)
		data_receive_expected = MAX_RX_SIZE;

	uart_irq_update(ser.uart_dev);
	while (uart_fifo_read(ser.uart_dev, &c, 1) > 0)
		continue;

	ser.data_recieved = 0;
	ser.data_notify_second = data_receive_expected;
	memset(ser.rx_buff, 0, MAX_RX_SIZE + 2);

	uart_irq_rx_enable(ser.uart_dev);
}

static void bt_uart_stop(void)
{
	uart_irq_rx_disable(ser.uart_dev);
}

static void bt_uart_isr(struct device *uart_dev)
{
//	struct ser_ctx *ser = user_data;

	if (uart_irq_update(ser.uart_dev) && uart_irq_rx_ready(ser.uart_dev)) {
		u8_t c;
		uart_fifo_read(ser.uart_dev, &c, 1);

		if (ser.data_recieved > MAX_RX_SIZE)
			return;

		ser.rx_buff[ser.data_recieved++] = c;

		if (ser.data_recieved == 1)
			k_stack_push(&ser.notify_stack, (stack_data_t)1);
		if (ser.data_recieved == ser.data_notify_second)
			k_stack_push(&ser.notify_stack, (stack_data_t)ser.data_notify_second);
	}
}

static void uart_receiver_send(const u8_t *buf, size_t size)
{

	__ASSERT(ser.uart_dev, "UART device is NULL");

	if (size == 0)
		return;

	do {
		uart_poll_out(ser.uart_dev, *buf++);
	} while (--size);
}

static void uart_receiver_send_string(const u8_t *buf)
{
	uart_receiver_send(buf, strlen(buf));
}

#define HM_PFX		"RFF: HM11: "

struct hm11_at_cmd {
	char	*at_command;
	char	*expected_answer;
	int	(*unexpected_handler)(char *expected, char *got);
};

static void hm11_do_reset(void)
{
	printk(HM_PFX "reset module: NOT Implemented!\n");
}

static int hm11_must_equal_h(char *expected, char *got)
{
	printk(HM_PFX "bad param, exp '%s', got '%s'\n", expected, got);
	return -EINVAL;
}

#define HM_SUPP_VERSION		524
#define HM_VERS_PREFIX		"HMSoft V"
#define HM_VERS_RESP		HM_VERS_PREFIX xstr(HM_SUPP_VERSION)

/* Allow any version >= 524 */
static int hm11_version_h(char *expected, char *got)
{
	size_t prefix_sz = strlen(HM_VERS_PREFIX);
	long hm_version;

	if (strncmp(got, HM_VERS_PREFIX, prefix_sz) != 0) {
		printk(HM_PFX "unexpected version respond: '%s'\n", got);

		return -EINVAL;
	}

	got += prefix_sz;
	hm_version = strtol(got, NULL, 10);
	printk(HM_PFX "firmware version: %ld\n", hm_version);

	if (hm_version >= HM_SUPP_VERSION)
		return 0;

	printk(HM_PFX "firmware version too old: %ld\n", hm_version);
	return -EINVAL;
}

struct hm11_at_cmd	hm11_assert[] = {
	{ "AT",		"OK",			hm11_must_equal_h },
	{ "AT+VERR?",	HM_VERS_RESP,		hm11_version_h },
	{ "AT+ADVI?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+ADTY?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+ALLO?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+IMME?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+MODE?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+NOTI?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+NAME?",	"OK+NAME:HMSoft\0",	hm11_must_equal_h },
	{ "AT+PIO1?",	"OK+Get:0",		hm11_must_equal_h }, //TODO switch to 1
	{ "AT+PASS?",	"OK+Get:999999",	hm11_must_equal_h },
	{ "AT+POWE?",	"OK+Get:2",		hm11_must_equal_h },
	{ "AT+PWRM?",	"OK+Get:1",		hm11_must_equal_h },
	{ "AT+ROLE?",	"OK+Get:0",		hm11_must_equal_h },
	{ "AT+TYPE?",	"OK+Get:3",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_pio1_reconf[] = {
	{ "AT+PIO11",	"OK+Set:1",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_type_reconf[] = {
	{ "AT+TYPE3",	"OK+Set:3",		hm11_must_equal_h },
	{ /* end of list */ }
};

#define WAIT_TIME	100
#define MAX_RETRY_AT	15

static int hm11_do_at_cmd(struct hm11_at_cmd *cmd_list)
{
	stack_data_t tmp;
	int retry = MAX_RETRY_AT;
	int reset_left = 0;
	int ret = 0;

	printk(HM_PFX "start AT cmd list %p\n", cmd_list);

	while (cmd_list->at_command) {
		char *exp_answer = cmd_list->expected_answer;
		size_t expected_answer_sz = strlen(exp_answer);

		bt_uart_reconfig(expected_answer_sz);
		uart_receiver_send_string(cmd_list->at_command);
		k_stack_pop(&ser.notify_stack, &tmp, K_MSEC(WAIT_TIME));
		k_stack_pop(&ser.notify_stack, &tmp, K_MSEC(WAIT_TIME));
		k_sleep(10);
		bt_uart_stop();

		/*
		 * HACK: sometimes HM-11 doesn't respond to AT command at all,
		 * confirmed with logic analyzer.
		 */
		if (!ser.data_recieved && retry) {
			printk(HM_PFX "answer lost for '%s', attempt %d\n",
				cmd_list->at_command, MAX_RETRY_AT - retry);
			retry--;

			continue;
		}

		/*
		 * HACK: if module still don't responding - try to reset it.
		 */
		if (!retry && !reset_left) {
			reset_left = 0;
			retry = MAX_RETRY_AT;
			hm11_do_reset();

			continue;
		}
		reset_left = 1;
		retry = MAX_RETRY_AT;

//		printk(HM_PFX "received: %s (%u)\n", (char *)ser.rx_buff, ser.data_recieved);

		if (strncmp(exp_answer, ser.rx_buff, MAX_RX_SIZE) != 0)
			ret = cmd_list->unexpected_handler(exp_answer, ser.rx_buff);

		if (ret)
			break;

		cmd_list++;
	}

	printk(HM_PFX "AT cmd list: %s\n", ret ? "FAIL" : "OK");

	return ret;
}

/*
 * Host CMD format:
 * ALARM:hh:mm - new alarm where hh:mm is wait time before alarm
 * ALARM:STOP - disarm current alarm
 * ALARM:INFO - info about current alarm
 */
#define HOSTCMD_PFX	"ALARM:"

static inline bool hm11_status_connected(void)
{
	//TODO: FIXME: check status led here
	return true;
}

static void hm11_send_cmd_respond(const char *str)
{
	if (hm11_status_connected())
		uart_receiver_send_string(str);
}

static int hm11_host_cmd_parse_chedule_alarm(struct host_cmd *cmd)
{
	size_t pfx_sz = strlen(HOSTCMD_PFX);
//	size_t cmd_sz = pfx_sz + 5;
	char *rx_buff_ptr = ser.rx_buff, *end;
	long alarm_hh, alarm_mm;
#define RESPOND_BUFF_SZ	24
	char str[RESPOND_BUFF_SZ];

	cmd->type = HOST_CMD_NONE;

	if (strncmp(HOSTCMD_PFX, ser.rx_buff, pfx_sz) != 0)
		goto bad_cmd;

	if (strlen(ser.rx_buff) < pfx_sz + 3)
		goto bad_cmd;

	rx_buff_ptr += pfx_sz;

	if (!strncmp("STOP", rx_buff_ptr, 4) != 0) {
		printk(HM_PFX "got hostcmd: stop current alarm\n");
		/* TODO: send respond later */
		hm11_send_cmd_respond("STOPPED");

		cmd->delay_sec = 0;
		cmd->type = HOST_CMD_DISARM_ALARM;
		return 0;
	}

	if (!strncmp("INFO", rx_buff_ptr, 4) != 0) {
		printk(HM_PFX "got hostcmd: get common info\n");
		/* TODO: send respond later */
		hm11_send_cmd_respond("NO-INFO");

		cmd->delay_sec = 0;
		cmd->type = HOST_CMD_COMMON_INFO;
		return 0;
	}

	alarm_hh = strtol(rx_buff_ptr, &end, 10);
	rx_buff_ptr = end + 1;
	alarm_mm = strtol(rx_buff_ptr, &end, 10);

	if (alarm_mm >= 60) {
		printk(HM_PFX "more than 60 minutes? Huh.\n");

		goto bad_cmd;
	}

	/* Just in case of TYPO in command */
	if (alarm_hh > 24) {
		printk(HM_PFX "more than 24 hours? Huh.\n");

		goto bad_cmd;
	}

	if ((alarm_hh * 60 + alarm_mm) * 60 < RISE_TIME_S) {
		printk(HM_PFX "less than rise time? Huh.\n");

		goto bad_cmd;
	}

	printk(HM_PFX "got hostcmd: wait for alarm for %ld:%ld\n", alarm_hh, alarm_mm);
	snprintf(str, RESPOND_BUFF_SZ, "WAIT %ld:%ld", alarm_hh, alarm_mm);
	/* TODO: send respond after alarm schedule */
	hm11_send_cmd_respond(str);

	cmd->delay_sec = (alarm_hh * 60 + alarm_mm) * 60;
	cmd->type = HOST_CMD_SCHEDULE_ALARM;
	return 0;

bad_cmd:
	printk(HM_PFX "junk instead of hostcmd: '%s'\n", ser.rx_buff);
	hm11_send_cmd_respond("NACK");
	return -EINVAL;
}

static int hm11_wait_for_host_cmd(struct host_cmd *cmd)
{
	stack_data_t tmp;
	size_t pfx_sz = strlen(HOSTCMD_PFX);
	size_t cmd_sz = pfx_sz + 5;

	bt_uart_reconfig(cmd_sz);
	k_stack_pop(&ser.notify_stack, &tmp, K_FOREVER);
	k_stack_pop(&ser.notify_stack, &tmp, K_MSEC(WAIT_TIME));
	k_sleep(10);
	bt_uart_stop();

	return hm11_host_cmd_parse_chedule_alarm(cmd);
}

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
	if (button_pressed())
		schedule_alarm_new(SLEEP_TIME);
	else
		check_for_alarm_powerfault();
}

static void xlate_leds(uint32_t val)
{
	uint32_t led0, led1;

	brightness_model->xlate_leds(val, &led0, &led1);
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

		if (button_pressed()) {
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

		if (button_pressed()) {
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
		led0 = TIM3->CCR4;
		led1 = TIM3->CCR3;

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
	while (!button_pressed())
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
	button_init();
	bt_uart_init();
	signal_led_init();
	timer3_pwm_init();

	brightness_model = get_model_logarithmic();
	brightness_model->init(RISE_TIME_S);

	sheck_for_alarm();

	hm11_do_at_cmd(hm11_assert);

	printk("Defined params: default SLEEP_TIME %u:%u:%u, RISE_TIME %u m, HOLD_TIME %u m\n",
			SLEEP_TIME / (60 * 60), (SLEEP_TIME % (60 * 60)) / 60,
			(SLEEP_TIME % (60 * 60)) % 60,
			RISE_TIME_S / 60, HOLD_TIME_S / 60);

	printk("RFF: button_pressed %d\n", button_pressed());

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
