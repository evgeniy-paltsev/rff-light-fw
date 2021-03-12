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

static int hm11_must_equal_h(char *cmd, char *expected, char *got, struct hm11_at_cmd *cmdlist)
{
	printk(HM_PFX "bad param on '%s': exp '%s', got '%s'\n", cmd, expected, got);
	return -EINVAL;
}

#define HM_SUPP_VERSION		524
#define HM_VERS_PREFIX		"HMSoft V"
#define HM_VERS_RESP		HM_VERS_PREFIX xstr(HM_SUPP_VERSION)

/* Allow any version >= 524 */
static int hm11_version_h(char *cmd, char *expected, char *got, struct hm11_at_cmd *cmdlist)
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

static int simple_reconfig_h(char *cmd, char *expected, char *got, struct hm11_at_cmd *cmdlist)
{
	int ret;

	printk(HM_PFX "bad param on '%s': exp '%s', got '%s', try to reconfigure\n", cmd, expected, got);
	if (!cmdlist)
		return -EINVAL;

	printk(HM_PFX "reconfigure command: '%s'\n", cmdlist[0].at_command);

	k_sleep(10);

	ret = hm11_do_at_cmd(cmdlist);
	printk(HM_PFX "reconfigure: %s\n", ret ? "FAIL" : "OK");

	return ret;
}

struct hm11_at_cmd	hm11_mode0_reconf[] = {
	{ "AT+MODE0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_noti0_reconf[] = {
	{ "AT+NOTI0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

#define HM11_MAME	"RFF_light"

struct hm11_at_cmd	hm11_name_reconf[] = {
	/* According to docs the answer should be "OK+SetName:" but actually it is "OK+Set:" */
	{ "AT+NAME"HM11_MAME,	"OK+Set:"HM11_MAME,		hm11_must_equal_h },
	{ /* end of list */ }
};

#define HM11_PASS	"181933"

struct hm11_at_cmd	hm11_pass_reconf[] = {
	{ "AT+PASS"HM11_PASS,	"OK+Set:"HM11_PASS,		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_pio1_reconf[] = {
	{ "AT+PIO11",	"OK+Set:1",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_advi0_reconf[] = {
	{ "AT+ADVI0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_adty0_reconf[] = {
	{ "AT+ADTY0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_allo0_reconf[] = {
	{ "AT+ALLO0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_imme0_reconf[] = {
	{ "AT+IMME0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_powe2_reconf[] = {
	{ "AT+POWE2",	"OK+Set:2",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_pwrm1_reconf[] = {
	{ "AT+PWRM1",	"OK+Set:1",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_role0_reconf[] = {
	{ "AT+ROLE0",	"OK+Set:0",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_type3_reconf[] = {
	{ "AT+TYPE3",	"OK+Set:3",		hm11_must_equal_h },
	{ /* end of list */ }
};

struct hm11_at_cmd	hm11_assert[] = {
	{ "AT",		"OK",			hm11_must_equal_h,	NULL },
	{ "AT+VERR?",	HM_VERS_RESP,		hm11_version_h,		NULL },
	{ "AT+ADVI?",	"OK+Get:0",		simple_reconfig_h,	hm11_advi0_reconf },
	{ "AT+ADTY?",	"OK+Get:0",		simple_reconfig_h,	hm11_adty0_reconf },
	{ "AT+ALLO?",	"OK+Get:0",		simple_reconfig_h,	hm11_allo0_reconf },
	{ "AT+IMME?",	"OK+Get:0",		simple_reconfig_h,	hm11_imme0_reconf },
	{ "AT+MODE?",	"OK+Get:0",		simple_reconfig_h,	hm11_mode0_reconf }, /* tested on 610 */
	{ "AT+NOTI?",	"OK+Get:0",		simple_reconfig_h,	hm11_noti0_reconf }, /* tested on 610 */
//	{ "AT+NAME?",	"OK+NAME:HMSoft",	hm11_must_equal_h,	NULL },
	{ "AT+NAME?",	"OK+NAME:"HM11_MAME,	simple_reconfig_h,	hm11_name_reconf },  /* tested on 610 */
	{ "AT+PIO1?",	"OK+Get:0",		hm11_must_equal_h,	NULL }, //TODO switch to 1
	{ "AT+PASS?",	"OK+Get:"HM11_PASS,	simple_reconfig_h,	hm11_pass_reconf },
	{ "AT+POWE?",	"OK+Get:2",		simple_reconfig_h,	hm11_powe2_reconf },
	{ "AT+PWRM?",	"OK+Get:1",		simple_reconfig_h,	hm11_pwrm1_reconf },
	{ "AT+ROLE?",	"OK+Get:0",		simple_reconfig_h,	hm11_role0_reconf },
	{ "AT+TYPE?",	"OK+Get:3",		simple_reconfig_h,	hm11_type3_reconf }, /* tested on 610 */
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
			ret = cmd_list->unexpected_handler(cmd_list->at_command, exp_answer, ser.rx_buff, cmd_list->cmdlist);

		if (ret)
			break;

		cmd_list++;
	}

	printk(HM_PFX "AT cmd list: %s\n", ret ? "FAIL" : "OK");

	return ret;
}

static inline bool hm11_status_connected(void)
{
	//TODO: FIXME: check status led here
	return true;
}

void hm11_send_cmd_respond(const char *str)
{
	if (hm11_status_connected())
		uart_receiver_send_string(str);
}

static int hm11_bad_hostcmd(bt_host_packet_t *pkt)
{
	printk(HM_PFX "junk instead of hostcmd: '%02x:%02x:%02x%02x%02x%02x'\n",
		pkt->packet_raw[0], pkt->packet_raw[1], pkt->packet_raw[2],
		pkt->packet_raw[3], pkt->packet_raw[4], pkt->packet_raw[5]);
	hm11_send_cmd_respond("NACK");

	return -EINVAL;
}

static int hm11_host_pkt_verify(bt_host_packet_t *pkt)
{
	char str[RESPOND_BUFF_SZ];

	if (pkt->packet_s.prefix != HOSTPKT_PFX) {
		printk(HM_PFX "hostcmd prefix mismatch\n");

		return hm11_bad_hostcmd(pkt);
	}

	if (pkt->packet_s.command == HOST_CMD_LAMP_MODE) {
		printk(HM_PFX "got hostcmd: use as a lamp\n");

		uint8_t type = pkt->packet_s.payload_lamp.lamp_type;

		if (type != LAMP_PARALLEL && type != LAMP_SEQUENTAL_COLD && type != LAMP_SEQUENTAL_WARM) {
			printk(HM_PFX "unknown lamp type\n");

			return hm11_bad_hostcmd(pkt);
		}

		if (pkt->packet_s.payload_lamp.brightnes > 100) {
			printk(HM_PFX "brightnes not in [0:100]? Huh.\n");

			return hm11_bad_hostcmd(pkt);
		}

		return 0;
	} else if (pkt->packet_s.command == HOST_CMD_PING) {
		printk(HM_PFX "got hostcmd: ping\n");
		hm11_send_cmd_respond("ALIVE");

		return 0;
	} else if (pkt->packet_s.command == HOST_CMD_SCHEDULE_ALARM) {
		uint32_t sleep_time_s = pkt->packet_s.payload_32b;

		if (sleep_time_s < RISE_TIME_S) {
			printk(HM_PFX "got hostcmd: set alarm: sleep time < rise time? Huh.\n");

			return hm11_bad_hostcmd(pkt);
		}

		/* Just in case of TYPO in command */
		if (sleep_time_s >= 24 * 60 * 60) {
			printk(HM_PFX "got hostcmd: set alarm: sleep time >= 24 hours? Huh.\n");

			return hm11_bad_hostcmd(pkt);
		}

		unsigned int alarm_hh, alarm_mm, alarm_ss;

		alarm_hh = sleep_time_s / (60 * 60);
		alarm_mm = (sleep_time_s % (60 * 60)) / 60;
		alarm_ss = (sleep_time_s % (60 * 60)) % 60;

		printk(HM_PFX "got hostcmd: wait for alarm for %uh %um %us (%us)\n",
			alarm_hh, alarm_mm, alarm_ss, sleep_time_s);
		snprintf(str, RESPOND_BUFF_SZ, "SLEEP %2u:%2u", alarm_hh, alarm_mm);
		/* TODO: send respond after alarm schedule */
		hm11_send_cmd_respond(str);

		return 0;
	} else if (pkt->packet_s.command == HOST_CMD_DISARM_ALARM) {
		printk(HM_PFX "got hostcmd: stop current alarm\n");
		/* TODO: send respond later */
		hm11_send_cmd_respond("STOPPED");

		return 0;
	} else if (pkt->packet_s.command == HOST_CMD_COMMON_INFO) {
		printk(HM_PFX "got hostcmd: get common info\n");

		/* we will send respond later */
		return 0;
	} else {
		printk(HM_PFX "got hostcmd: unknown\n");
	}

	return hm11_bad_hostcmd(pkt);
}

int hm11_wait_for_host_pkt(bt_host_packet_t *pkt)
{
	stack_data_t tmp;
	size_t cmd_sz = sizeof(bt_host_packet_t);

	bt_uart_reconfig(cmd_sz);
	k_stack_pop(&ser.notify_stack, &tmp, K_FOREVER);
	k_stack_pop(&ser.notify_stack, &tmp, K_MSEC(WAIT_TIME));
	/* We've recieved full packet here so we can stop BT imidiately */
	bt_uart_stop();

	memcpy(pkt, ser.rx_buff, cmd_sz);

	return hm11_host_pkt_verify(pkt);
}

static void hm11_reset_init(void)
{

}

static void hm11_reset(void)
{
	/* TODO: reset here */
}

int hm11_init(void)
{
	printk(HM_PFX "password for paring is \'"HM11_PASS"\'\n");
	hm11_reset_init();
	hm11_reset();

	return hm11_do_at_cmd(hm11_assert);
}
