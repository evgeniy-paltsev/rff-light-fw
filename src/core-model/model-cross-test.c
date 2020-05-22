#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "core-model.h"

/******************************************************************************/
/* Simple small values */

#define A__WAIT_TIME		60
#define A__RISE_TIME		40
#define A__HOLD_MAX_TIME	30
#define A__DECR_TO_MID_TIME	10
#define A__HOLD_MID_TIME	70
#define A__DISARM_HOLD_MID_TIME	50
#define A__DECR_TO_ZERO_TIME	20

static void set_const_params_a(struct core_model_params *params)
{
	params->wait_time = A__WAIT_TIME;
	params->rise_time = A__RISE_TIME;
	params->hold_max_time = A__HOLD_MAX_TIME;
	params->decrease_to_mid_time = A__DECR_TO_MID_TIME;
	params->hold_mid_time = A__HOLD_MID_TIME;
	params->disarm_hold_mid_time = A__DISARM_HOLD_MID_TIME;
	params->decrease_to_zero_time = A__DECR_TO_ZERO_TIME;
}

static void check_a(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	time_sec = 60;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 61;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_curr_value ==  time_sec - A__WAIT_TIME);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 99;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 39);
	assert(io.brightnes_curr_value == A__RISE_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 100;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 101;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 129;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 130;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 131;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 141;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 209;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 210;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 211;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 229;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_curr_value == A__DECR_TO_ZERO_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 230;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 231;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_partial_1(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 100;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 101;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 129;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 130;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 131;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 209;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 210;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 229;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_curr_value == A__DECR_TO_ZERO_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 230;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 231;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_before_trigger(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* disarming start */
	time_sec = 5;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 6;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 14;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 15;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 16;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 64;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 65;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 66;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 84;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 85;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_before_trigger_2(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	/* disarming start */
	time_sec = 0;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 6;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 6);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 9;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 10;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 60;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 61;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 79;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 0);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 80;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_rise(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	time_sec = 60;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 61;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_curr_value ==  time_sec - A__WAIT_TIME);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* disarming start */
	time_sec = 62;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 63;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 71;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 72;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 73;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 121;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 122;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 123;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 141;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 62);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 142;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_rise_2(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	/* disarming start */
	time_sec = 60;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 69;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 70;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 119;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 120;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 121;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 60);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_hold_max(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	time_sec = 60;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 61;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_curr_value ==  time_sec - A__WAIT_TIME);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 99;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 39);
	assert(io.brightnes_curr_value == A__RISE_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* hold max started */
	time_sec = 100;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 101;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* disarming start */
	time_sec = 102;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 103;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 111;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 112;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 113;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 161;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 162;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 163;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 181;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 102);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 182;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_hold_max_2(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 99;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 39);
	assert(io.brightnes_curr_value == A__RISE_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* hold max started */
	/* disarming start */
	time_sec = 100;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 100);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 109;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 100);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 110;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 100);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 161;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 100);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 179;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 100);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 180;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_decr_to_mid(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	time_sec = 60;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 61;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_curr_value ==  time_sec - A__WAIT_TIME);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 99;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 39);
	assert(io.brightnes_curr_value == A__RISE_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* hold max started */
	time_sec = 100;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 101;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 129;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* decr to mid started */
	time_sec = 130;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 131;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	/* disarming start */
	time_sec = 132;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	/* note: disarm_time_adjustment differs from current time */
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 2);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 133;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 3);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 141;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 189;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 190;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 191;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 209;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 210;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_decr_to_mid_2(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 129;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* decr to mid started */
	/* disarming start */
	time_sec = 130;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!io.finished);
	/* note: disarm_time_adjustment differs from current time */
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 133;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 3);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 209;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 130);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 210;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_hold_mid(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 59;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	time_sec = 60;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 61;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_curr_value ==  time_sec - A__WAIT_TIME);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 99;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__RISE_TIME);
	assert(io.brightnes_curr_value == 39);
	assert(io.brightnes_curr_value == A__RISE_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* hold max started */
	time_sec = 100;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 101;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 129;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	/* decr to mid started */
	time_sec = 130;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 131;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	/* hold mid started */
	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 141;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	/* disarming start */
	time_sec = 142;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!(io.disarm_info & A_DISARM_PRIMARY));
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 142);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 143;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 142);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 161;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 142);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 162;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_hold_mid_2(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 139;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	/* hold mid started */
	/* disarming start */
	time_sec = 140;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!(io.disarm_info & A_DISARM_PRIMARY));
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 140);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 159;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 140);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 162;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_while_hold_mid_3(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	/* hold mid started */
	time_sec = 140;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 141;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	/* hold mid finished */
	/* disarming start */
	time_sec = 209;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!(io.disarm_info & A_DISARM_PRIMARY));
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 209);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 210;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 209);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 228;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 209);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 229;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

static void check_a_disarm_before_trigger__wsecondary(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_a(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* disarming start */
	time_sec = 5;
	assert(!(io.disarm_info & A_DISARM_NEW));
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!(io.disarm_info & A_DISARM_SECONDARY));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 6;
	io.disarm_info |= A_DISARM_NEW; /* false one, must be ignored */
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!(io.disarm_info & A_DISARM_SECONDARY));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 14;
	io.disarm_info |= A_DISARM_NEW; /* false one, must be ignored */
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(!(io.disarm_info & A_DISARM_SECONDARY));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 5);
	assert(io.brightnes_period == A__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 9);
	assert(io.brightnes_curr_value == A__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_THIS_OR_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	/* second disarming start */
	time_sec = 15;
	io.disarm_info |= A_DISARM_NEW;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 15);
	assert(io.disarm_time_adjustment == time_sec);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 16;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 15);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 19;
	io.disarm_info |= A_DISARM_NEW; /* false one, must be ignored */
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 15);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 4);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 34;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(!io.finished);
	assert(io.disarm_time_adjustment == 15);
	assert(io.brightnes_period == A__DECR_TO_ZERO_TIME);
	//printf("%d\n", io.brightnes_curr_value);
	assert(io.brightnes_curr_value == 19);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 35;
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 255;
	io.disarm_info |= A_DISARM_NEW; /* false one, must be ignored */
	do_alarm_model(time_sec, &io, &params);
	assert(!(io.disarm_info & A_DISARM_NEW));
	assert(io.disarm_info & A_DISARM_PRIMARY);
	assert(io.disarm_info & A_DISARM_SECONDARY);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 256;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/
/* Real values ( 7h 30min ) */

#define B__WAIT_TIME		27000 // ((7 * 60 + 30) * 60)
#define B__RISE_TIME		1200  // (20 * 60)
#define B__HOLD_MAX_TIME	900   // (15 * 60)
#define B__DECR_TO_MID_TIME	50
#define B__HOLD_MID_TIME	3600  // (60 * 60)
#define B__DECR_TO_ZERO_TIME	60

static void set_const_params_b(struct core_model_params *params)
{
	params->wait_time = B__WAIT_TIME;
	params->rise_time = B__RISE_TIME;
	params->hold_max_time = B__HOLD_MAX_TIME;
	params->decrease_to_mid_time = B__DECR_TO_MID_TIME;
	params->hold_mid_time = B__HOLD_MID_TIME;
	params->decrease_to_zero_time = B__DECR_TO_ZERO_TIME;
}

static void check_b(void)
{
	struct core_model_io io;
	struct core_model_params params;
	uint32_t time_sec;

	io.disarm_info = A_DISARM_NONE;
	io.disarm_time_adjustment = 0;
	set_const_params_b(&params);

	time_sec = 0;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 1;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 26999;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	/* alarm triggered */
	time_sec = 27000;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__RISE_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 27001;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__RISE_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_curr_value ==  time_sec - B__WAIT_TIME);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 28199;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__RISE_TIME);
	assert(io.brightnes_curr_value == 1199);
	assert(io.brightnes_curr_value == B__RISE_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 28200;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 28201;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 29099;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MAX);

	time_sec = 29100;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 29101;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 29149;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__DECR_TO_MID_TIME);
	assert(io.brightnes_curr_value == 49);
	assert(io.brightnes_curr_value == B__DECR_TO_MID_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MAX);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 29150;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 29151;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 32749;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_MID);

	time_sec = 32750;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 0);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 32751;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 32809;
	do_alarm_model(time_sec, &io, &params);
	assert(!io.finished);
	assert(io.brightnes_period == B__DECR_TO_ZERO_TIME);
	assert(io.brightnes_curr_value == 59);
	assert(io.brightnes_curr_value == B__DECR_TO_ZERO_TIME - 1);
	assert(io.brightnes_from == BRIGHTNESS_MID);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 32810;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 32811;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 32812;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);

	time_sec = 0xFFFFFFFF;
	do_alarm_model(time_sec, &io, &params);
	assert(io.finished);
	assert(io.brightnes_from == BRIGHTNESS_OFF);
	assert(io.brightnes_to == BRIGHTNESS_OFF);
}

/******************************************************************************/

int main(void)
{
	check_a();
	check_a_partial_1();
	/* first disarm check */
	check_a_disarm_before_trigger();
	check_a_disarm_before_trigger_2();
	check_a_disarm_while_rise();
	check_a_disarm_while_rise_2();
	check_a_disarm_while_hold_max();
	check_a_disarm_while_hold_max_2();
	check_a_disarm_while_decr_to_mid();
	check_a_disarm_while_decr_to_mid_2();
	check_a_disarm_while_hold_mid();
	check_a_disarm_while_hold_mid_2();
	check_a_disarm_while_hold_mid_3();
	/* second disarm check */
	check_a_disarm_before_trigger__wsecondary();

	check_b();

	printf("OK\n");

	return 0;
}
