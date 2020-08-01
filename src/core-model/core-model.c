#include "core-model.h"

/* NORMAL:
 *
 * brightness
 *     ^
 * MAX_|           _____
 *     |          /     \
 * MID_|         /       \__________
 *     |        /                   \
 *   0_|_______/                     \_
 *     ----------------------------------------> time
 *     ^       ^         ^            ^
 *     |       |         |            |
 *     |       |         |            off (finished)
 *     |       |         hold finished
 *     |       triggered
 *     scheduled
 *
 *
 * DISARM (x):--
 *             |
 * brightness  |
 *     ^      /
 * MAX_|     /
 *     |    |
 * MID_|    |  __________
 *     |    v /          \
 *   0_|____x/            \_
 *     ----------------------------------------> time
 *     ^    ^
 *     |    |
 *     |    |
 *     |    |
 *     |    triggered (expected to be)
 *     scheduled
 *
 *
 * DISARM (x):--
 *             |
 * brightness  |
 *     ^      /
 * MAX_|     /
 *     |    |
 * MID_|    |
 *     |    v
 *   0_|____x_______________
 *     ----------------------------------------> time
 *     ^       ^
 *     |       |
 *     |       |
 *     |       |
 *     |       triggered (expected to be)
 *     scheduled
 *
 *
 * DISARM (x):-----
 *                |
 * brightness     |
 *     ^          |
 * MAX_|          v
 *     |          x
 * MID_|         / \__________
 *     |        /             \
 *   0_|_______/               \_
 *     ----------------------------------------> time
 *     ^       ^
 *     |       |
 *     |       |
 *     |       |
 *     |       triggered
 *     scheduled
 *
 *
 */

static void do_alarm_model_direct(const uint32_t rtc_time,
				  struct core_model_io *io,
				  const struct core_model_params *params);
static void do_alarm_model_disarmed_pre_cancel(const uint32_t rtc_time,
					       struct core_model_io *io,
					       const struct core_model_params *params);
static void do_alarm_model_disarmed_early(const uint32_t rtc_time,
					  struct core_model_io *io,
					  const struct core_model_params *params);
static void do_alarm_model_disarmed_late(const uint32_t rtc_time,
					 struct core_model_io *io,
					 const struct core_model_params *params);

static void set_const_brightness(struct core_model_io *io,
				 const enum brightnes_value_ops brightnes)
{
	io->brightnes_period = 0;
	io->brightnes_curr_value = 0;
	io->brightnes_from = brightnes;
	io->brightnes_to = brightnes;
}

static void set_alarm_finished(struct core_model_io *io)
{
	io->atomic_alarm_info |= A_ALARM_FINISHED;
	log_info("ALARM: mark as finished\n");
}

bool alarm_finished(struct core_model_io *io)
{
	return io->atomic_alarm_info & A_ALARM_FINISHED;
}

void set_disarm_new(struct core_model_io *io)
{
	io->atomic_alarm_info |= A_DISARM_NEW;
}

static void disarm_adjust_secondary(const uint32_t adjustment,
				    struct core_model_io *io)
{
	dynamic_assert(!(io->atomic_alarm_info & A_DISARM_SECONDARY));

	log_info("ALARM: mark secondary disarm with adjustment %u\n", adjustment);

	//printf("disarm_time_adjustment %u rtc_time -\n", io->disarm_time_adjustment);

	io->atomic_alarm_info |= A_DISARM_SECONDARY;
	io->disarm_time_adjustment = adjustment;
}

static void disarm_adjust_primary(const uint32_t adjustment,
				  struct core_model_io *io)
{
	dynamic_assert(!(io->atomic_alarm_info & A_DISARM_PRIMARY));
	dynamic_assert(io->disarm_time_adjustment == 0);

	log_info("ALARM: mark primary disarm with adjustment %u\n", adjustment);

	io->atomic_alarm_info |= A_DISARM_PRIMARY;
	io->disarm_time_adjustment = adjustment;
}

static void disarm_adjust_pre_cancel(struct core_model_io *io)
{
	dynamic_assert(!(io->atomic_alarm_info & A_DISARM_PRE_CANCEL));
	dynamic_assert(io->disarm_time_adjustment == 0);

	log_info("ALARM: mark pe-cancel disarm\n");

	io->atomic_alarm_info |= A_DISARM_PRE_CANCEL;
}

static void do_alarm_finished(struct core_model_io *io)
{
	io->atomic_alarm_info &= ~A_DISARM_NEW;
	set_const_brightness(io, BRIGHTNESS_OFF);
}

static void process_new_disarm(const uint32_t rtc_time,
			       struct core_model_io *io,
			       const struct core_model_params *params)
{
	io->atomic_alarm_info &= ~A_DISARM_NEW;

	/* all possible disarms are done */
	if (io->atomic_alarm_info & A_DISARM_SECONDARY)
		return;

	/* pre-cancel doesn't have any additional disarm */
	if (io->atomic_alarm_info & A_DISARM_PRE_CANCEL)
		return;

	if (io->atomic_alarm_info & A_DISARM_PRIMARY) {
		/* determine disarm flags depending on the current primary disarm state */

		dynamic_assert(!(io->atomic_alarm_info & A_DISARM_SECONDARY));

		uint32_t model_time = rtc_time - io->disarm_time_adjustment;
		uint32_t time_since_disarmed_to_mid = model_time - params->decrease_to_mid_time;

		if (time_since_disarmed_to_mid < params->disarm_hold_mid_time) {
			disarm_adjust_secondary(rtc_time, io);
		}
	} else {
		/* determine disarm flags depending on the current direct state */

		uint32_t time_since_triggered = rtc_time - params->wait_time;
		uint32_t time_after_max_brght = time_since_triggered - params->rise_time;
		uint32_t time_since_decr_mid = time_after_max_brght - params->hold_max_time;
		uint32_t time_after_mid_brght = time_since_decr_mid - params->decrease_to_mid_time;

		/* alarm is not triggered - it's pre-cancel */
		if (rtc_time < params->wait_time) {
			disarm_adjust_pre_cancel(io);
		/*
		 * time_since_triggered = ammount of time (sec) after alarm was triggered
		 * effective in interval [0; params->rise_time)
		 */
		} else if (time_since_triggered < params->rise_time) {
			disarm_adjust_primary(rtc_time, io);
		/*
		 * time_after_max_brght = ammount of time (sec) after max brightnes achieved while increasing
		 * effective in interval [0; params->hold_max_time)
		 */
		} else if (time_after_max_brght < params->hold_max_time) {
			disarm_adjust_primary(rtc_time, io);
		/*
		 * time_since_decr_mid = ammount of time (sec) after initial decrease from max brightnes to middle
		 * effective in interval [0; params->decrease_to_mid_time)
		 */
		} else if (time_since_decr_mid < params->decrease_to_mid_time) {
			disarm_adjust_primary(rtc_time - time_since_decr_mid, io);
		/*
		 * time_after_mid_brght = ammount of time (sec) after mid brightnes achieved while decreasing
		 * effective in interval [0; params->hold_mid_time)
		 */
		} else if (time_after_mid_brght < params->hold_mid_time) {
			disarm_adjust_secondary(rtc_time, io);
		/*
		 * time_since_decr_zero = ammount of time (sec) after initial decrease
		 * from middle brightnes to zero
		 * effective in interval [0; params->decrease_to_zero_time)
		 */
		}

		/* no man land */
	}
}

static inline bool no_disarm_pending(struct core_model_io *io)
{
	if (io->atomic_alarm_info & A_DISARM_NEW)
		return false;

	if (io->atomic_alarm_info & A_DISARM_PRE_CANCEL)
		return false;

	if (io->atomic_alarm_info & A_DISARM_PRIMARY)
		return false;

	if (io->atomic_alarm_info & A_DISARM_SECONDARY)
		return false;

	return true;
}

void do_alarm_model(const uint32_t rtc_time,
		    struct core_model_io *io,
		    const struct core_model_params *params)
{
	if (io->atomic_alarm_info & A_ALARM_FINISHED) {
		do_alarm_finished(io);

		return;
	}

	if (io->atomic_alarm_info & A_DISARM_NEW)
		process_new_disarm(rtc_time, io, params);

	if (no_disarm_pending(io)) {
		do_alarm_model_direct(rtc_time, io, params);

		return;
	}

	if (io->atomic_alarm_info & A_DISARM_SECONDARY) {
		do_alarm_model_disarmed_late(rtc_time, io, params);

		return;
	}

	if (io->atomic_alarm_info & A_DISARM_PRIMARY) {
		do_alarm_model_disarmed_early(rtc_time, io, params);

		return;
	}

	if (io->atomic_alarm_info & A_DISARM_PRE_CANCEL) {
		do_alarm_model_disarmed_pre_cancel(rtc_time, io, params);

		return;
	}
}

static void do_alarm_model_disarmed_late(const uint32_t rtc_time,
					 struct core_model_io *io,
					 const struct core_model_params *params)
{
	uint32_t model_time;

	//printf("disarm_time_adjustment %u rtc_time %u\n", io->disarm_time_adjustment, rtc_time);

	dynamic_assert(rtc_time >= io->disarm_time_adjustment);
	model_time = rtc_time - io->disarm_time_adjustment;

	if (model_time < params->decrease_to_zero_time) {
		log_if_zero(model_time, "ALARM: disarmed_late: start decreasing brightness to ZERO for %us at %us\n", params->decrease_to_zero_time, rtc_time);
		io->brightnes_period = params->decrease_to_zero_time;
		io->brightnes_curr_value = model_time;
		io->brightnes_from = BRIGHTNESS_MID;
		io->brightnes_to = BRIGHTNESS_OFF;

		return;
	}

	/* anything after decrease from middle to zero is always zero */
	log_info("ALARM: disarmed_late: mark alarm finished at %u\n", rtc_time);
	set_const_brightness(io, BRIGHTNESS_OFF);
	set_alarm_finished(io);

	return;
}

static void do_alarm_model_disarmed_early(const uint32_t rtc_time,
					  struct core_model_io *io,
					  const struct core_model_params *params)
{
	uint32_t model_time;

	dynamic_assert(rtc_time >= io->disarm_time_adjustment);
	model_time = rtc_time - io->disarm_time_adjustment;

	//printf("model_time %u rtc_time %u\n", model_time, rtc_time);

	/*
	 * TODO: do we want here dynamic forward time instead of constant?
	 * Most likely yes, but it's really rear case, so le't keep it as
	 * a TODO forever :)
	 */
	if (model_time < params->decrease_to_mid_time) {
		log_if_zero(model_time, "ALARM: disarmed_early: start [in/de]creasing brightness to MID for %us at %us\n", params->decrease_to_mid_time, rtc_time);
		io->brightnes_period = params->decrease_to_mid_time;
		io->brightnes_curr_value = model_time;
		io->brightnes_from = BRIGHTNESS_THIS_OR_OFF;
		io->brightnes_to = BRIGHTNESS_MID;

		return;
	}

	uint32_t time_since_disarmed_to_mid = model_time - params->decrease_to_mid_time;

	if (time_since_disarmed_to_mid < params->disarm_hold_mid_time) {
		log_if_zero(time_since_disarmed_to_mid, "ALARM: disarmed_early: start holding brightness at MID for %us at %us\n", params->disarm_hold_mid_time, rtc_time);
		set_const_brightness(io, BRIGHTNESS_MID);

		return;
	}

	uint32_t time_since_decr_zero = time_since_disarmed_to_mid - params->disarm_hold_mid_time;

	/*
	 * time_since_decr_zero = ammount of time (sec) after initial decrease
	 * from middle brightnes to zero
	 * effective in interval [0; params->decrease_to_zero_time)
	 */
	if (time_since_decr_zero < params->decrease_to_zero_time) {
		log_if_zero(time_since_decr_zero, "ALARM: disarmed_early: start decreasing brightness to ZERO for %us at %us\n", params->decrease_to_zero_time, rtc_time);
		io->brightnes_period = params->decrease_to_zero_time;
		io->brightnes_curr_value = time_since_decr_zero;
		io->brightnes_from = BRIGHTNESS_MID;
		io->brightnes_to = BRIGHTNESS_OFF;

		return;
	}

	/* anything after decrease from middle to zero is zero */
	log_info("ALARM: disarmed_early: mark alarm finished at %u\n", rtc_time);
	set_const_brightness(io, BRIGHTNESS_OFF);
	set_alarm_finished(io);

	return;
}

static void do_alarm_model_disarmed_pre_cancel(const uint32_t rtc_time,
					       struct core_model_io *io,
					       const struct core_model_params *params)
{
	/* immediately go to zero */
	log_info("ALARM: disarmed_pre_cancel: mark alarm finished at %u\n", rtc_time);
	set_const_brightness(io, BRIGHTNESS_OFF);
	set_alarm_finished(io);

	return;
}

/*
 * @rtc_time - correct time (ticked even in power down state).
 *   Dynamic input.
 */
static void do_alarm_model_direct(const uint32_t rtc_time,
				  struct core_model_io *io,
				  const struct core_model_params *params)
{
	if (rtc_time < params->wait_time) {
		log_if_zero(rtc_time, "ALARM: direct: start waiting %us before trigger\n", params->wait_time);
		set_const_brightness(io, BRIGHTNESS_OFF);

		return;
	}

	/* alarm was triggered */

	uint32_t time_since_triggered = rtc_time - params->wait_time;

	/*
	 * time_since_triggered = ammount of time (sec) after alarm was triggered
	 * effective in interval [0; params->rise_time)
	 */
	if (time_since_triggered < params->rise_time) {
		log_if_zero(time_since_triggered, "ALARM: direct: start increasing brightness (rise) to MAX for %us at %us\n", params->rise_time, rtc_time);
		io->brightnes_period = params->rise_time;
		io->brightnes_curr_value = time_since_triggered;
		io->brightnes_from = BRIGHTNESS_OFF;
		io->brightnes_to = BRIGHTNESS_MAX;

		return;
	}

	uint32_t time_after_max_brght = time_since_triggered - params->rise_time;

	/*
	 * time_after_max_brght = ammount of time (sec) after max brightnes achieved while increasing
	 * effective in interval [0; params->hold_max_time)
	 */
	if (time_after_max_brght < params->hold_max_time) {
		log_if_zero(time_after_max_brght, "ALARM: direct: start holding MAX brightness for %us at %us\n", params->hold_max_time, rtc_time);
		set_const_brightness(io, BRIGHTNESS_MAX);

		return;
	}

	uint32_t time_since_decr_mid = time_after_max_brght - params->hold_max_time;

	/*
	 * time_since_decr_mid = ammount of time (sec) after initial decrease from max brightnes to middle
	 * effective in interval [0; params->decrease_to_mid_time)
	 */
	if (time_since_decr_mid < params->decrease_to_mid_time) {
		log_if_zero(time_since_decr_mid, "ALARM: direct: start decreasing brightness to MID for %us at %us\n", params->decrease_to_mid_time, rtc_time);
		io->brightnes_period = params->decrease_to_mid_time;
		io->brightnes_curr_value = time_since_decr_mid;
		io->brightnes_from = BRIGHTNESS_MAX;
		io->brightnes_to = BRIGHTNESS_MID;

		return;
	}

	uint32_t time_after_mid_brght = time_since_decr_mid - params->decrease_to_mid_time;

	/*
	 * time_after_mid_brght = ammount of time (sec) after mid brightnes achieved while decreasing
	 * effective in interval [0; params->hold_mid_time)
	 */
	if (time_after_mid_brght < params->hold_mid_time) {
		log_if_zero(time_after_mid_brght, "ALARM: direct: start holding MID brightness for %us at %us\n", params->hold_mid_time, rtc_time);
		set_const_brightness(io, BRIGHTNESS_MID);

		return;
	}

	uint32_t time_since_decr_zero = time_after_mid_brght - params->hold_mid_time;

	/*
	 * time_since_decr_zero = ammount of time (sec) after initial decrease
	 * from middle brightnes to zero
	 * effective in interval [0; params->decrease_to_zero_time)
	 */
	if (time_since_decr_zero < params->decrease_to_zero_time) {
		log_if_zero(time_since_decr_zero, "ALARM: direct: start decreasing brightness to ZERO for %us at %us\n", params->decrease_to_zero_time, rtc_time);
		io->brightnes_period = params->decrease_to_zero_time;
		io->brightnes_curr_value = time_since_decr_zero;
		io->brightnes_from = BRIGHTNESS_MID;
		io->brightnes_to = BRIGHTNESS_OFF;

		return;
	}

	/* anything after decrease from middle to zero is zero */
	log_info("ALARM: direct: mark alarm finished at %u\n", rtc_time);
	set_const_brightness(io, BRIGHTNESS_OFF);
	set_alarm_finished(io);

	return;
}
