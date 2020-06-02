#include "rtc-ctl.h"

#define LSE_TIMEOUT	0xFFFF

static void backup_alarm(uint32_t wait_seconds);

static void disable_clk_gate_and_protection(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_BKPEN;
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;

	//enable backup area write
	PWR->CR |= PWR_CR_DBP;
}

static int lse_enable(void)
{
	uint32_t timeout = LSE_TIMEOUT;
	RCC->BDCR |= RCC_BDCR_LSEON;
	while (!(RCC->BDCR & RCC_BDCR_LSERDY) && timeout--)
		__NOP();

	if (timeout)
		return 0;

	return -1;
}

static int lse_init(void)
{
	uint32_t attempts = 5;

	if (RCC->BDCR & RCC_BDCR_LSEON) {
		printk("LSE is enabled, skip init\n");

		return 0;
	}

	while (attempts--) {
		if (!lse_enable()) {
			printk("LSE enabled\n");

			return 0;
		}

		printk("LSE enable fail, attempt left: %u\n", attempts);
	}

	return -1;
}

#define RCC_BDCR_RTCSEL_MASK	RCC_BDCR_RTCSEL_HSE

#define RTC_TIMEOUT		0xFFFF

static void backup_domain_init(uint16_t atomic_state)
{
	atomic_state_set(atomic_state);
	backup_data_set(0);
	backup_alarm(0xFFFFFFFF);
}

static void rtc_configure(uint16_t atomic_state)
{
	uint32_t timeout = RTC_TIMEOUT;

	/* wait till last operation finished */
	while (!(RTC->CRL & RTC_CRL_RTOFF) && timeout--)
		__NOP();

	RTC->CRL |= RTC_CRL_CNF;

	/* fTR_CLK = fRTCCLK/(PRL[19:0]+1) */
	RTC->PRLH = 0;
	RTC->PRLL = 32767;

	RTC->CNTH = 0;
	RTC->CNTL = 0;

	/* alarm flag reset */
	RTC->CRL &= ~RTC_CRL_ALRF;

	RTC->CRL &= ~RTC_CRL_CNF;

	while (!(RTC->CRL & RTC_CRL_RTOFF) && timeout--)
		__NOP();

	backup_domain_init(atomic_state);

	printk("RTC configuration %s, timeout val: %u\n", timeout ? "OK" : "FAIL", timeout);
}

static int rtc_enable(uint16_t atomic_state)
{
	uint32_t timeout = RTC_TIMEOUT;

	if (RCC->BDCR & RCC_BDCR_RTCEN)  {
		printk("RTC is enabled, skip init\n");

		return 0;
	}

	if ((RCC->BDCR & RCC_BDCR_RTCSEL_MASK) != RCC_BDCR_RTCSEL_LSE) {
		printk("RTCSEL is wrong, reprogram\n");
		RCC->BDCR &= ~RCC_BDCR_RTCSEL_MASK;
		RCC->BDCR |= RCC_BDCR_RTCSEL_LSE;
	}

	if ((RCC->BDCR & RCC_BDCR_RTCEN) != RCC_BDCR_RTCEN) {
		printk("RTC is disabled, reprogram\n");
		RCC->BDCR |= RCC_BDCR_RTCEN;
		rtc_configure(atomic_state);
	} else {
		/* wait for read synchronization is done */
		while (!(RTC->CRL & RTC_CRL_RSF) && timeout--)
			__NOP();
	}

	return 0;
}

static void clock_init(uint16_t atomic_state)
{
	if (!lse_init()) {
		rtc_enable(atomic_state);
	}
}

void rtc_init(uint16_t atomic_state)
{
	disable_clk_gate_and_protection();
	clock_init(atomic_state);
}

static void backup_alarm(uint32_t wait_seconds)
{
	BKP->DR1 = (uint16_t)(wait_seconds >> 16);
	BKP->DR2 = (uint16_t)(wait_seconds);
}

void rtc_set_alarm(uint32_t wait_seconds)
{
	uint32_t timeout = RTC_TIMEOUT;

	backup_alarm(wait_seconds);

	/* wait till last operation finished */
	while (!(RTC->CRL & RTC_CRL_RTOFF) && timeout--)
		__NOP();

	RTC->CRL |= RTC_CRL_CNF;

	RTC->CNTH = 0;
	RTC->CNTL = 0;

	RTC->ALRH = (uint16_t)(wait_seconds >> 16);
	RTC->ALRL = (uint16_t)(wait_seconds);

	/* alarm flag reset */
	RTC->CRL &= ~RTC_CRL_ALRF;

	RTC->CRL &= ~RTC_CRL_CNF;

	while (!(RTC->CRL & RTC_CRL_RTOFF) && timeout--)
		__NOP();

	printk("RTC set_alarm, curr time -> %u, wait for -> %u: %s\n",
			rtc_get_time(), rtc_get_alarm(),
			timeout ? "OK" : "FAIL");
}

/* TODO: support CNTL overflow */
uint32_t rtc_get_time(void)
{
	return RTC->CNTH << 16 | RTC->CNTL;
}

uint32_t rtc_get_alarm(void)
{
	/* ALARM registers are not readable so read alarm value from backup */
	return (uint32_t)(BKP->DR1) << 16 | BKP->DR2;
}

bool rtc_is_alarm(void)
{
	uint32_t timeout = RTC_TIMEOUT;

	while (!(RTC->CRL & RTC_CRL_RSF) && timeout--)
		__NOP();

#ifdef HW_ALARM_CHECK
	return (RTC->CRL & RTC_CRL_ALRF) == RTC_CRL_ALRF;
#endif
	return rtc_get_time() >= rtc_get_alarm();
}

void atomic_state_set(uint16_t data)
{
	BKP->DR3 = data;
}

uint16_t atomic_state_get(void)
{
	return (uint16_t)(BKP->DR3);
}

void backup_data_set(uint32_t data)
{
	BKP->DR4 = (uint16_t)(data >> 16);
	BKP->DR5 = (uint16_t)(data);
}

uint32_t backup_data_get(void)
{
	return (uint32_t)(BKP->DR4) << 16 | BKP->DR5;
}
