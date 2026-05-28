/**
 ******************************************************************************
 * @file    : ADC.c
 * @brief   : ADC init, calibration, and conversion functions
 * project  : EE 329 S'26 A5
 * authors  : Aaron Price Jr. & Brandon Valenti
 * version  : 0.2
 * date     : 2026-05-19
 * target   : NUCLEO-L4A6ZG
 *
 * Hardware:
 *   PA0 (A0) -- ADC1 channel 5, analog input
 *               Connected to Q1 emitter / top of RE (10Ω sense resistor)
 *   VREF+    -- 3.3V internal reference
 ******************************************************************************
 */

#include "ADC.h"

/* Shared with ADC1_2_IRQHandler in main.c */
volatile uint16_t adc_result   = 0;
volatile uint8_t  adc_eoc_flag = 0;

/* Calibration struct - populated by two-point cal routine in main.c */
ADC_Cal_t adc_cal = {0};

void ADC_init(void)
{
    /* -- PA0: analog mode, no pull --
     * PA0 = ADC1_IN5, connected to Q1 emitter (top of RE = 10Ω)          */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER |=  GPIO_MODER_MODE0;     /* 11 = analog mode             */
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;     /* no pull-up / pull-down       */

    /* -- ADC clock: synchronous, HCLK/1 (24 MHz) --                       */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    ADC123_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos);

    /* -- Power up ADC voltage regulator --
     * Must exit deep-power-down and wait ≥20 us for regulator to settle   */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    delay_us(20);                           /* regulator settle time        */

    /* -- Hardware self-calibration (single-ended) --
     * Corrects for internal offset errors; must run before ADEN           */
    ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_5;   /* channel 5 single-ended      */
    ADC1->CR     &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
    ADC1->CR     |=   ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }    /* wait for cal to finish       */

    /* -- Enable ADC --                                                      */
    ADC1->ISR |=  ADC_ISR_ADRDY;           /* clear ready flag             */
    ADC1->CR  |=  ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { } /* wait until ready           */
    ADC1->ISR |=  ADC_ISR_ADRDY;           /* clear ready flag             */

    /* -- Sequence: 1 conversion, channel 5 (PA0) --                        */
    ADC1->SQR1 &= ~ADC_SQR1_L;             /* L=0: one conversion          */
    ADC1->SQR1 &= ~ADC_SQR1_SQ1;
    ADC1->SQR1 |=  (5U << ADC_SQR1_SQ1_Pos); /* SQ1 = channel 5           */

    /* -- Sample time: 47.5 cycles (0b100) for channel 5 --
     * Suitable for low-impedance sources; increase to 0b111 (640.5 cycles)
     * if source impedance is high (>10kΩ)                                  */
    ADC1->SMPR1 &= ~ADC_SMPR1_SMP5;
    ADC1->SMPR1 |=  (2U << ADC_SMPR1_SMP5_Pos);

    /* -- Single conversion, software trigger, 12-bit resolution --          */
    ADC1->CFGR  &= ~(ADC_CFGR_CONT | ADC_CFGR_EXTEN | ADC_CFGR_RES);

    /* -- Enable EOC interrupt --
     * ADC1_2_IRQHandler in main.c stores result and sets adc_eoc_flag     */
    ADC1->IER |=  ADC_IER_EOCIE;
    ADC1->ISR |=  ADC_ISR_EOC;              /* clear any pending flag       */

    NVIC_EnableIRQ(ADC1_2_IRQn);
    __enable_irq();
}

/*
 * ADC_read_blocking
 * Triggers one conversion and blocks until EOC ISR sets adc_eoc_flag.
 * Returns raw 12-bit count.
 */
uint16_t ADC_read_blocking(void)
{
    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;
    while (!adc_eoc_flag) { }
    adc_eoc_flag = 0;
    return adc_result;
}

/*
 * ADC_average_counts
 * Returns the average of n consecutive blocking conversions.
 * Used during two-point calibration to reduce noise.
 */
uint16_t ADC_average_counts(uint16_t n)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < n; i++)
    {
        sum += ADC_read_blocking();
    }
    return (uint16_t)(sum / n);
}

/*
 * ADC_apply_cal
 * Converts a raw 12-bit ADC count to millivolts using two-point
 * calibration coefficients stored in adc_cal.
 *
 * Formula:
 *   mv = (raw - offset_counts) * gain_num_mv / gain_den_counts
 *
 * Falls back to ideal conversion if calibration not yet performed.
 * Output clamped to [0, VREF_MV].
 */
int32_t ADC_apply_cal(uint16_t raw)
{
    int32_t mv;

    if (adc_cal.valid)
    {
        /* Two-point calibrated conversion */
        mv = ((int32_t)raw - adc_cal.offset_counts)
             * adc_cal.gain_num_mv
             / adc_cal.gain_den_counts;
    }
    else
    {
        /* Fallback: ideal 12-bit conversion, no calibration */
        mv = (int32_t)raw * VREF_MV / ADC_FULL_SCALE;
    }

    if (mv < 0)       mv = 0;
    if (mv > VREF_MV) mv = VREF_MV;

    return mv;
}
