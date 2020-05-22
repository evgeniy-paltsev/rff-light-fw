#ifndef BT_HM11_CTL_H_
#define BT_HM11_CTL_H_

#include <soc.h>

enum host_cmd_type {
	HOST_CMD_NONE = 0,
	HOST_CMD_SCHEDULE_ALARM,
	HOST_CMD_DISARM_ALARM,
	HOST_CMD_COMMON_INFO,
};

#define RESPOND_BUFF_SZ		27

struct hm11_at_cmd {
	char	*at_command;
	char	*expected_answer;
	int	(*unexpected_handler)(char *cmd, char *expected, char *got);
};

struct host_cmd {
	enum host_cmd_type	type;
	u32_t			delay_sec;
};

extern struct hm11_at_cmd hm11_assert[];


int hm11_wait_for_host_cmd(struct host_cmd *cmd);
void bt_uart_init(void);
int hm11_do_at_cmd(struct hm11_at_cmd *cmd_list);
void hm11_send_cmd_respond(const char *str);

#endif
