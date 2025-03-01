#ifndef ADC_H
#define ADC_H

/**
 * @brief   Low level ADC driver initialization.
 *
 * @notapi
 */
/**
 * @brief   Number of channels in an injected conversion sequence.
 */
#define ADC_JSQR_NUM_CH(n)      ((n) - 1)

#define ADC_JSQR_JSQ1_N(n)      ((n) << 8)  /**< @brief 1st channel in seq. */
#define ADC_JSQR_JSQ2_N(n)      ((n) << 14) /**< @brief 2st channel in seq. */
#define ADC_JSQR_JSQ3_N(n)      ((n) << 20) /**< @brief 3st channel in seq. */
#define ADC_JSQR_JSQ4_N(n)      ((n) << 26) /**< @brief 4st channel in seq. */

#define ADC_JSQR_JEXTSEL_SRC(n) ((n) << 2)  /**< @brief external trigger selector. */
#define ADC_JSQR_JEXTENP(n)     ((n) << 6) /**< @brief external trigger polarity. */
/** @} */

/**
 * @name    Sampling rate settings helper macros
 * @{
 */
#define ADC_SMPR1_SMP_AN0(n)    ((n) << 0)  /**< @brief AN0 sampling time.  */
#define ADC_SMPR1_SMP_AN1(n)    ((n) << 3)  /**< @brief AN1 sampling time.  */
#define ADC_SMPR1_SMP_AN2(n)    ((n) << 6)  /**< @brief AN2 sampling time.  */
#define ADC_SMPR1_SMP_AN3(n)    ((n) << 9)  /**< @brief AN3 sampling time.  */
#define ADC_SMPR1_SMP_AN4(n)    ((n) << 12) /**< @brief AN4 sampling time.  */
#define ADC_SMPR1_SMP_AN5(n)    ((n) << 15) /**< @brief AN5 sampling time.  */
#define ADC_SMPR1_SMP_AN6(n)    ((n) << 18) /**< @brief AN6 sampling time.  */
#define ADC_SMPR1_SMP_AN7(n)    ((n) << 21) /**< @brief AN7 sampling time.  */
#define ADC_SMPR1_SMP_AN8(n)    ((n) << 24) /**< @brief AN8 sampling time.  */
#define ADC_SMPR1_SMP_AN9(n)    ((n) << 27) /**< @brief AN9 sampling time.  */

/**
 * @name    Sampling rates
 * @{
 */
#define ADC_SMPR_SMP_1P5        0   /**< @brief 14 cycles conversion time   */
#define ADC_SMPR_SMP_2P5        1   /**< @brief 15 cycles conversion time.  */
#define ADC_SMPR_SMP_4P5        2   /**< @brief 17 cycles conversion time.  */
#define ADC_SMPR_SMP_7P5        3   /**< @brief 20 cycles conversion time.  */
#define ADC_SMPR_SMP_19P5       4   /**< @brief 32 cycles conversion time.  */
#define ADC_SMPR_SMP_61P5       5   /**< @brief 74 cycles conversion time.  */
#define ADC_SMPR_SMP_181P5      6   /**< @brief 194 cycles conversion time. */
#define ADC_SMPR_SMP_601P5      7   /**< @brief 614 cycles conversion time. */
/**
 * @name    Available analog channels
 * @{
 */
#define ADC_CHANNEL_IN0         0   /**< @brief External analog input 0.    */
#define ADC_CHANNEL_IN1         1   /**< @brief External analog input 1.    */
#define ADC_CHANNEL_IN2         2   /**< @brief External analog input 2.    */
#define ADC_CHANNEL_IN3         3   /**< @brief External analog input 3.    */
#define ADC_CHANNEL_IN4         4   /**< @brief External analog input 4.    */
#define ADC_CHANNEL_IN5         5   /**< @brief External analog input 5.    */
#define ADC_CHANNEL_IN6         6   /**< @brief External analog input 6.    */
#define ADC_CHANNEL_IN7         7   /**< @brief External analog input 7.    */
#define ADC_CHANNEL_IN8         8   /**< @brief External analog input 8.    */
#define ADC_CHANNEL_IN9         9   /**< @brief External analog input 9.    */
#define ADC_CHANNEL_IN10        10  /**< @brief External analog input 10.   */
#define ADC_CHANNEL_IN11        11  /**< @brief External analog input 11.   */
#define ADC_CHANNEL_IN12        12  /**< @brief External analog input 12.   */
#define ADC_CHANNEL_IN13        13  /**< @brief External analog input 13.   */
#define ADC_CHANNEL_IN14        14  /**< @brief External analog input 14.   */
#define ADC_CHANNEL_IN15        15  /**< @brief External analog input 15.   */
#define ADC_CHANNEL_IN16        16  /**< @brief External analog input 16.   */
#define ADC_CHANNEL_IN17        17  /**< @brief External analog input 17.   */
#define ADC_CHANNEL_IN18        18  /**< @brief External analog input 18.   */

static void adc_lld_vreg_on(ADC_TypeDef *adcm) {
  adcm->CR = 0;   /* RM 12.4.3.*/
  adcm->CR = ADC_CR_ADVREGEN_0;
  osalSysPolledDelayX(OSAL_US2RTC(STM32_HCLK, 10));
}
static void adc_lld_analog_on(ADC_TypeDef *adcm) {
  adcm->CR |= ADC_CR_ADEN;
  while ((adcm->ISR & ADC_ISR_ADRD) == 0);
}

static void adc_lld_calibrate(ADC_TypeDef *adcm) {
  adcm->CR |= ADC_CR_ADCAL;
  while ((adcm->CR & ADC_CR_ADCAL) != 0);
}

static void adc_lld_init(void) {
  nvicEnableVector(STM32_ADC1_NUMBER, STM32_ADC_ADC12_IRQ_PRIORITY);

  rccEnableADC12(FALSE);
  rccResetADC12();
  ADC1_2_COMMON->CCR = 0;

  rccEnableADC34(FALSE);
  rccResetADC34();
  ADC3_4_COMMON->CCR = 0;

  adc_lld_vreg_on(ADC1);
  adc_lld_calibrate(ADC1);
  adc_lld_analog_on(ADC1);
//ADC1
  ADC1->ISR   = ADC1->ISR;
  ADC1->IER  |= ADC_IER_JEOSIE;
  ADC1->SMPR1 = ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_7P5) |
                ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_7P5);

  ADC1->JSQR  = ADC_JSQR_JSQ1_N(ADC_CHANNEL_IN1) |
                ADC_JSQR_JSQ2_N(ADC_CHANNEL_IN2) |
                ADC_JSQR_JEXTSEL_SRC(0) | ADC_JSQR_JEXTENP(2) |
                ADC_JSQR_NUM_CH(2);

  ADC1->CFGR  = 0;
  ADC1->CR   |= ADC_CR_JADSTART;

  adc_lld_vreg_on(ADC2);
  adc_lld_calibrate(ADC2);
  adc_lld_analog_on(ADC2);
//ADC2
  ADC2->ISR = ADC2->ISR;
  ADC2->SMPR1 = ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_7P5) |
                ADC_SMPR1_SMP_AN2(ADC_SMPR_SMP_7P5);

  ADC2->JSQR  = ADC_JSQR_JSQ1_N(ADC_CHANNEL_IN1) |
                ADC_JSQR_JSQ2_N(ADC_CHANNEL_IN2) |
                ADC_JSQR_JEXTSEL_SRC(0) | ADC_JSQR_JEXTENP(2) |
                ADC_JSQR_NUM_CH(2);

  ADC2->CFGR  = 0;
  ADC2->CR   |= ADC_CR_JADSTART;

//ADC3
  adc_lld_vreg_on(ADC3);
  adc_lld_calibrate(ADC3);
  adc_lld_analog_on(ADC3);
  ADC3->ISR   = ADC3->ISR;
  ADC3->SMPR1 = ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_19P5);
  ADC3->JSQR  = ADC_JSQR_JSQ1_N(ADC_CHANNEL_IN1) |
                ADC_JSQR_JEXTSEL_SRC(0) | ADC_JSQR_JEXTENP(2) |
                ADC_JSQR_NUM_CH(1);

  ADC3->CFGR  = 0;
  ADC3->CR   |= ADC_CR_JADSTART;

//ADC4
  adc_lld_vreg_on(ADC4);
  adc_lld_calibrate(ADC4);
  adc_lld_analog_on(ADC4);
  ADC4->ISR   = ADC4->ISR;
  ADC4->SMPR1 = ADC_SMPR1_SMP_AN3(ADC_SMPR_SMP_7P5) |
                ADC_SMPR1_SMP_AN4(ADC_SMPR_SMP_7P5);
  ADC4->JSQR  = ADC_JSQR_JSQ1_N(ADC_CHANNEL_IN3) |
                ADC_JSQR_JSQ2_N(ADC_CHANNEL_IN4) |
                ADC_JSQR_JEXTSEL_SRC(0) | ADC_JSQR_JEXTENP(2) |
                ADC_JSQR_NUM_CH(2);

  /* ADC configuration.*/
  ADC4->CFGR = 0;
  ADC4->CR  |= ADC_CR_JADSTART;
}

struct fo_filter {
    float a, b;
    float x1, y1;
    fo_filter(const float value, const float beta) : x1(value), y1(value) {
        a = (beta - 2.0f) / (beta + 2.0f);
        b = beta / (beta + 2.0f);
    }
    float value() {return y1;}
    void run(const float x) {
        y1 = b * (x + x1) - a * y1;
        x1 = x;
    }
};

#define ADC_VOLTAGE (3.3f)
#define ADC_BITS (12)
#define ADC_RANGE (1U << ADC_BITS)
#define SHUNT_SENSE (45e-3f)
#define CURRENT_SCALE ((ADC_VOLTAGE/ADC_RANGE)/SHUNT_SENSE)

#define VOLTAGE_R2 (5.11f)
#define VOLTAGE_R1 (47.5f)
#define VOLTAGE_SCALE ((ADC_VOLTAGE/ADC_RANGE)/VOLTAGE_R2*(VOLTAGE_R2+VOLTAGE_R1))

#define TEMP_V0 (1.055f)
#define TEMP_DVDT (0.023f)
#define TEMP_T0 (25.0f)
#define TEMP_SCALE ((ADC_VOLTAGE/ADC_RANGE)/TEMP_DVDT)
#define TEMP_OFFSET (TEMP_V0/TEMP_DVDT-TEMP_T0)

struct ADC {
    float Ia_offset;
    float Ib_offset;
    void init() {
        adc_lld_init();
        fo_filter offset_a(ADC_RANGE * 0.5f * CURRENT_SCALE, 0.01f),
                  offset_b(ADC_RANGE * 0.5f * CURRENT_SCALE, 0.01f);
        Ia_offset = Ib_offset = 0.0f;
        for (unsigned i = 0; i < 10000; ++i) {
            chEvtWaitAny(ALL_EVENTS);
            offset_a.run(Ia());
            offset_b.run(Ib());
        }
        Ia_offset = offset_a.value();
        Ib_offset = offset_b.value();
    }
    float Ia() {
        unsigned x = ADC1->JDR1;
        float y = x * CURRENT_SCALE;
        return y - Ia_offset;
    }
    float Ib() {
        unsigned x = ADC2->JDR1;
        float y = x * CURRENT_SCALE;
        return y - Ib_offset;
    }
    float Va() {return ADC1->JDR2 * VOLTAGE_SCALE;}
    float Vb() {return ADC2->JDR2 * VOLTAGE_SCALE;}
    float Vc() {return ADC4->JDR2 * VOLTAGE_SCALE;}
    float Vbus() {return ADC4->JDR1 * VOLTAGE_SCALE;}
    float T() {return ADC3->JDR1 * TEMP_SCALE - TEMP_OFFSET;}
};

#endif // ADC_H
