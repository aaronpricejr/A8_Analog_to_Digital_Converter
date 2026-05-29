/**
 ******************************************************************************
 * @file    : ADC.c
 * @brief   : ADC init and conversion functions
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

void ADC_init(void)
{
    /* -- PA0: analog mode, no pull -- */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER |=  GPIO_MODER_MODE0;     /* 11 = analog mode */
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;     /* no pull-up / pull-down */

    /* -- ADC clock: synchronous, HCLK/1, 24 MHz -- */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    ADC123_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos);

    /* -- Power up ADC voltage regulator -- */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    delay_us(20);

    /* -- Hardware self-calibration, single-ended -- */
    ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_5;
    ADC1->CR     &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
    ADC1->CR     |=   ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }

    /* -- Enable ADC -- */
    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR  |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    ADC1->ISR |= ADC_ISR_ADRDY;

    /* -- Sequence: 1 conversion, channel 5 PA0 -- */
    ADC1->SQR1 &= ~ADC_SQR1_L;
    ADC1->SQR1 &= ~ADC_SQR1_SQ1;
    ADC1->SQR1 |=  (5U << ADC_SQR1_SQ1_Pos);

    /* -- Sample time for channel 5 -- */
    ADC1->SMPR1 &= ~ADC_SMPR1_SMP5;
    ADC1->SMPR1 |=  (2U << ADC_SMPR1_SMP5_Pos);

    /* -- Single conversion, software trigger, 12-bit resolution -- */
    ADC1->CFGR &= ~(ADC_CFGR_CONT | ADC_CFGR_EXTEN | ADC_CFGR_RES);

    /* -- Enable EOC interrupt -- */
    ADC1->IER |= ADC_IER_EOCIE;
    ADC1->ISR |= ADC_ISR_EOC;

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
 * ADC_counts_to_mv
 * Converts raw 12-bit ADC count to millivolts using ideal conversion.
 *
 * Formula:
 *   mv = raw * VREF_MV / ADC_FULL_SCALE
 */
int32_t ADC_counts_to_mv(uint16_t raw)
{
    int32_t mv = ((int32_t)raw * 10000 - 17796) / 12361; //calibration

    if (mv < 0)       mv = 0;
    if (mv > VREF_MV) mv = VREF_MV;

    return mv;
}
