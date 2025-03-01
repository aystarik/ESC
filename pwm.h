#ifndef PWM_H
#define PWM_H

#define FOC_PWM_CLK (144*1000*1000)
#define FOC_PWM_FREQ (72*1000)
#define FOC_PWM_PERIOD (FOC_PWM_CLK/FOC_PWM_FREQ)

static void pwm_init(void) {
  /* Clock activation and timer reset.*/
  rccEnableTIM1(FALSE);
  rccResetTIM1();

  /* All channels configured in PWM1 mode with preload enabled and will
     stay that way until the driver is stopped.*/
  STM32_TIM1->CCMR1 = STM32_TIM_CCMR1_OC1M(6) | STM32_TIM_CCMR1_OC1PE |
                     STM32_TIM_CCMR1_OC2M(6) | STM32_TIM_CCMR1_OC2PE;
  STM32_TIM1->CCMR2 = STM32_TIM_CCMR2_OC3M(6) | STM32_TIM_CCMR2_OC3PE |
                     STM32_TIM_CCMR2_OC4M(6) | STM32_TIM_CCMR2_OC4PE;
  /* Driver re-configuration scenario, it must be stopped first.*/
  STM32_TIM1->CR1    = 0; /* Timer disabled. */
  STM32_TIM1->CCR[0] = 0; /* Comparator 1 disabled. */
  STM32_TIM1->CCR[1] = 0; /* Comparator 2 disabled. */
  STM32_TIM1->CCR[2] = 0; /* Comparator 3 disabled. */
  STM32_TIM1->CCR[3] = 0; /* Comparator 4 disabled. */
  STM32_TIM1->CNT  = 0; /* Counter reset to zero. */

  /* Timer configuration.*/
  STM32_TIM1->PSC  = 0; // full FOC_PWM_CLK
  STM32_TIM1->ARR  = FOC_PWM_PERIOD - 1; // 2000 for 72/36 kHz
  STM32_TIM1->CR2  = STM32_TIM_CR2_MMS(7); //CC4REF is ADC trigger

  /* Output enables and polarities setup.*/
  STM32_TIM1->CCER = STM32_TIM_CCER_CC1E | STM32_TIM_CCER_CC2E | STM32_TIM_CCER_CC3E;

  STM32_TIM1->EGR   = STM32_TIM_EGR_UG;      /* Update event.                */
  STM32_TIM1->SR    = 0;                     /* Clear pending IRQs.          */
  STM32_TIM1->DIER  = 0;
  STM32_TIM1->CR1 = STM32_TIM_CR1_CMS(1);
  STM32_TIM1->BDTR  = STM32_TIM_BDTR_MOE;
}

static void pwm_start(void) {
  STM32_TIM1->CR1 |= STM32_TIM_CR1_ARPE | STM32_TIM_CR1_URS |
                     STM32_TIM_CR1_CEN;
  STM32_TIM1->RCR = 1; // write to RCR after CEN in CR1 to get updates in a valley
}

static inline void pwm_set_channel(unsigned channel, unsigned width) {
  STM32_TIM1->CCR[channel] = width;
}
#endif // PWM_H
