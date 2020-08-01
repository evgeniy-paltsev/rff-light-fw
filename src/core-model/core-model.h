#ifndef CORE_MODEL_H_
#define CORE_MODEL_H_

#include <stdint.h>
#include <stdbool.h>

#include "../brightness-ops.h"


#ifdef VERIF_LAUNCH
#include <assert.h>
#define BIT(n)			(1UL << (n))
#define log_if_zero(value, fmt, ...)
#define log_info(fmt, ...)
#define dynamic_assert assert

#else
#include <sys/printk.h>
#include <sys/util.h>
#include "../dynamic-assert.h"
#define log_if_zero(value, fmt, ...) ({ if (!value) printk(fmt, ##__VA_ARGS__); })
#define log_info(fmt, ...) ({ printk(fmt, ##__VA_ARGS__); })

#endif


enum disarm_options {
	A_DISARM_NEW				= BIT(0), /* public, W/O */
	/* primary triggered */
	A_DISARM_PRIMARY			= BIT(1), /* private, sticky */
	/* secondary triggered */
	A_DISARM_SECONDARY			= BIT(2), /* private, sticky */
	/* disarm alarm before it trigger */
	A_DISARM_PRE_CANCEL			= BIT(3), /* private, sticky */
	/* @A_ALARM_FINISHED - current alarm finished, we can stop alarm thread
	 * and clean all resources */
	A_ALARM_FINISHED			= BIT(4), /* public, R/O */
	/* @A_ALARM_NEW - value for new alarm start */
	A_ALARM_NEW				= 0,
	/* @A_ALARM_POR_VALUE - value for first POR (when RTC isn't initialized) */
	A_ALARM_POR_VALUE			= A_ALARM_FINISHED,
};

//__attribute__((deprecated))

struct core_model_params {
	/* @wait_time - initial time (sec) before current alarm
	 *  wait_time = sleep_time - rise_time
	 *  !!! backed up in battery domain memory !!! */
	uint32_t wait_time;
	/* @rise_time - most likely compile-time constant */
	uint32_t rise_time;
	/* @hold_max_time - hold maximum brightnes time (sec) when we've reached
	 *                  it after alarm was triggered
	 *  hold_max_time - most likely compile-time constant */
	uint32_t hold_max_time;
	/* @decrease_to_mid_time - most likely compile-time constant */
	uint32_t decrease_to_mid_time;
	/* @hold_mid_time - hold middle brightnes time (sec) when we've reached
	 *                  it after decreasing from maximum
	 *  hold_mid_time - most likely compile-time constant
	 *  It's used for both normal operation and disarm */
	uint32_t hold_mid_time;
	/* @decrease_to_zero_time - most likely compile-time constant */
	uint32_t decrease_to_zero_time;

	/* @disarm_forward_time - time (sec) from initial disarm to brightnes
	 *                        adjusted to middle
	 *  disarm_forward_time - most likely compile-time constant */
	uint32_t disarm_hold_mid_time;
};

struct core_model_io {
	/* input & output - RW */
	/*  !!! backed up in battery domain memory !!! */
	uint16_t atomic_alarm_info;
	/*  !!! backed up in battery domain memory !!! */
	uint32_t disarm_time_adjustment;

	/* output values - W - are always overwritten */
	uint32_t brightnes_period;
	uint32_t brightnes_curr_value;
	enum brightnes_value_ops brightnes_from;
	enum brightnes_value_ops brightnes_to;
};

void do_alarm_model(const uint32_t rtc_time, struct core_model_io *io, const struct core_model_params *params);
bool alarm_finished(struct core_model_io *io);
void set_disarm_new(struct core_model_io *io);

#endif
