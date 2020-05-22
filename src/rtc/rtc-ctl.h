#ifndef RTC_CTL_H_
#define RTC_CTL_H_

#include <soc.h>
#include <sys/printk.h>

void rtc_init(void);

void rtc_set_alarm(uint32_t wait_seconds);
bool rtc_is_alarm(void);
uint32_t rtc_get_time(void);
uint32_t rtc_get_alarm(void);

void atomic_state_set(uint16_t data);
uint16_t atomic_state_get(void);

void backup_data_set(uint32_t data);
uint32_t backup_data_get(void);

#endif
