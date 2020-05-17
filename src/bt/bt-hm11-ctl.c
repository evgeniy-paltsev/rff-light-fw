#include "bt-hm11-ctl.h"

#include <drivers/uart.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../alarm-params.h"

#define str(a) #a
#define xstr(a) str(a)

#define HM_PFX		"RFF: HM11: "

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

void bt_uart_init(void)
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

int hm11_do_at_cmd(struct hm11_at_cmd *cmd_list)
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

	if (alarm_hh < 0 || alarm_mm < 0) {
		printk(HM_PFX "negative time? Huh.\n");

		goto bad_cmd;
	}

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
	snprintf(str, RESPOND_BUFF_SZ, "WAIT %2ld:%2ld", alarm_hh, alarm_mm);
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

int hm11_wait_for_host_cmd(struct host_cmd *cmd)
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
