#ifndef ALARM_PARAMS_H_
#define ALARM_PARAMS_H_

#ifdef CONFIG_ALARM_TESTING_SESSION

#define RISE_TIME_S		30
#define HOLD_MAXT_S		30
#define HOLD_MIDT_S		40

#else

#define RISE_TIME_S		(20 * 60)
#define HOLD_MAXT_S		(22 * 60)
#define HOLD_MIDT_S		(60 * 60)

#endif /* CONFIG_ALARM_TESTING_SESSION */

#define SLEEP_TIME_MIN		30
#define SLEEP_TIME_HOURS	7

#define SLEEP_TIME_S		((SLEEP_TIME_HOURS * 60 + SLEEP_TIME_MIN) * 60)

#endif
