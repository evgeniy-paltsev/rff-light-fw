#ifndef BT_HM11_CTL_H_
#define BT_HM11_CTL_H_

#include <soc.h>

#define RESPOND_BUFF_SZ		27


// packet format:
//
// uint8_t cmd[6];
//
// cmd[0] == 'A'
// cmd[1] <= host_cmd_type
// cmd[2:5] <= payload, 4 bytes, little endian
//
// cmd[1] == HOST_CMD_NONE
// invalid command
//
// cmd[1] == HOST_CMD_PING
// payload isn't checked
//
// cmd[1] == HOST_CMD_SCHEDULE_ALARM
// cmd[2:5] => time to sleep in seconds, should be
//  - more than RISE_TIME_S (architecture restriction)
//  - less than 24 hour (native requirement)
//
// cmd[1] == HOST_CMD_DISARM_ALARM
// payload isn't checked
//
// cmd[1] == HOST_CMD_COMMON_INFO
// payload isn't checked
//
// cmd[1] == HOST_CMD_LAMP_MODE
// cmd[2] => lamp type
//  - 0 = parallel
//  - 1 = sequental, cold first
//  - 2 = sequental, warm first
// cmd[3] => brightnes in persents, should be less than 100%

#define HOSTPKT_PFX	'A'

enum host_cmd_type {
	HOST_CMD_NONE = 0,
	HOST_CMD_LAMP_MODE,
	HOST_CMD_PING,
	HOST_CMD_SCHEDULE_ALARM,
	HOST_CMD_DISARM_ALARM,
	HOST_CMD_COMMON_INFO,
};

enum lamp_type {
	LAMP_PARALLEL = 0,
	LAMP_SEQUENTAL_COLD,
	LAMP_SEQUENTAL_WARM
};

#define BT_H_PKT_SZ		6

typedef union bt_host_packet {
	uint8_t packet_raw[BT_H_PKT_SZ];
	struct __packed {
		uint8_t prefix;
		uint8_t command;
		union {
			uint32_t payload_32b;
			uint8_t payload_raw[4];
			struct __packed {
				uint8_t lamp_type;
				uint8_t brightnes;
				uint8_t unused[2];
			} payload_lamp;
		};
	} packet_s;
} bt_host_packet_t;

BUILD_ASSERT(sizeof(bt_host_packet_t) == BT_H_PKT_SZ);

struct hm11_at_cmd {
	char	*at_command;
	char	*expected_answer;
	int	(*unexpected_handler)(char *cmd, char *expected, char *got, struct hm11_at_cmd *cmdlist);
	struct hm11_at_cmd *cmdlist;
};

struct host_cmd {
	enum host_cmd_type	type;
	u32_t			cmd_u32_param_0;
};

extern struct hm11_at_cmd hm11_assert[];


int hm11_wait_for_host_pkt(bt_host_packet_t *pkt);
void bt_uart_init(void);
int hm11_do_at_cmd(struct hm11_at_cmd *cmd_list);
void hm11_send_cmd_respond(const char *str);
int hm11_init(void);

#endif
