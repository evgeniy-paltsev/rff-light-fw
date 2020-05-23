#include "tim3-pwm.h"

/* PWM on PB0, PB1 */
static void timer3_pwm_gpio_init(void)
{
	//Enable GPIO "B" port
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;


	//PWB OUT (PB0) init
	GPIOB->CRL &= ~GPIO_CRL_CNF0;
	//AF, Output, PP
	GPIOB->CRL |= GPIO_CRL_CNF0_1;
	//Output TX - 50MHz
	GPIOB->CRL |= GPIO_CRL_MODE0;


	//PWB OUT (PB1) init
	GPIOB->CRL &= ~GPIO_CRL_CNF1;
	//AF, Output, PP
	GPIOB->CRL |= GPIO_CRL_CNF1_1;
	//Output TX - 50MHz
	GPIOB->CRL |= GPIO_CRL_MODE1;
}

void timer3_pwm_init(void)
{
	//clock TIM3
	RCC->APB1ENR |=	RCC_APB1ENR_TIM3EN;
	//prescaller: clk = clk / (PSC+1);
	TIM3->PSC = 0;
	//Frequency
	TIM3->ARR = 0xFFFF;

	//������� ����� ��������������� ������ �������� ����������������
	TIM3->CR1 |=	TIM_CR1_ARPE;

	//������� ����� ��������������� �������� �������� ���������
//	TIM3->CCMR1 |=	TIM_CCMR1_OC1PE;
//	TIM3->CCMR1 |=	TIM_CCMR1_OC2PE;
	TIM3->CCMR2 |=	TIM_CCMR2_OC3PE;
	TIM3->CCMR2 |=	TIM_CCMR2_OC4PE;
	//OC2M = 110 - PWM mode 1
//	TIM3->CCMR1 |=	(TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1);
//	TIM3->CCMR1 |=	(TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1);
	TIM3->CCMR2 |=	(TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1);
	TIM3->CCMR2 |=	(TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1);
	//channels as output
//	TIM3->CCMR1 &=	~TIM_CCMR1_CC1S;
//	TIM3->CCMR1 &=	~TIM_CCMR1_CC2S;
	TIM3->CCMR2 &=	~TIM_CCMR2_CC3S;
	TIM3->CCMR2 &=	~TIM_CCMR2_CC4S;


	//Duty cycle = 0% impulse length
//	TIM3->CCR1 = 0;
//	TIM3->CCR2 = 0;
	TIM3->CCR3 = 0;
	TIM3->CCR4 = 0;

	//TIM3 output enable
//	TIM3->CCER |= TIM_CCER_CC1E;
//	TIM3->CCER |= TIM_CCER_CC2E;
	TIM3->CCER |= TIM_CCER_CC3E;
	TIM3->CCER |= TIM_CCER_CC4E;

	//Output polarity
//	TIM3->CCER &= ~TIM_CCER_CC1P;
//	TIM3->CCER &= ~TIM_CCER_CC2P;
	TIM3->CCER &= ~TIM_CCER_CC3P;
	TIM3->CCER &= ~TIM_CCER_CC4P;

	//enable TIM3
	TIM3->CR1 |= TIM_CR1_CEN;

	timer3_pwm_gpio_init();
}

uint32_t get_curr_pwm_value(void)
{
	return (uint32_t)TIM3->CCR4 + (uint32_t)TIM3->CCR3;
}
