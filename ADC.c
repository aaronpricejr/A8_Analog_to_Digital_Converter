/**
 ******************************************************************************
 * @file    : ADC.c
 * @brief   : ADC initialization and configuration
 * project  : EE 329 S'26 A5
 * authors  : Aaron Price Jr. & Brandon Valenti
 * version  : 0.1
 * date     : 2026-05-19
 * compiler : STM32CubeIDE v.1.19.0
 * target   : NUCLEO-L4A6ZG
 ******************************************************************************
 */

#include "ADC.h"

volatile uint16_t adc_result   = 0;
volatile uint8_t  adc_eoc_flag = 0;

void delay_us(uint32_t us)
{
    for (volatile uint32_t i = 0; i < (us * 4); i++) { ; }
}

void ADC_init(void)
{
    /* Clock & power */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    ADC123_COMMON->CCR |= (1 << ADC_CCR_CKMODE_Pos);
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    delay_us(20);

    /* Calibration */
    ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_5;
    ADC1->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
    ADC1->CR |=  ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { ; }

    /* Enable ADC */
    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR  |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { ; }
    ADC1->ISR |= ADC_ISR_ADRDY;

    /* Sampling & sequencing */
    ADC1->SQR1  |= (5 << ADC_SQR1_SQ1_Pos);
    ADC1->SMPR1 |= (1 << ADC_SMPR1_SMP5_Pos);
    ADC1->CFGR  &= ~(ADC_CFGR_CONT | ADC_CFGR_EXTEN | ADC_CFGR_RES);

    /* EOC interrupt */
    ADC1->IER |= ADC_IER_EOCIE;
    ADC1->ISR |= ADC_ISR_EOC;
    NVIC->ISER[0] = (1 << (ADC1_2_IRQn & 0x1F));
    __enable_irq();

    /* GPIO PA0 analog input */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER  |= GPIO_MODER_MODE0;

    /* Start first conversion */
    ADC1->CR |= ADC_CR_ADSTART;
}
