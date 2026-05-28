#include "ADC.h"

volatile uint16_t adc_result = 0;
volatile uint8_t adc_eoc_flag = 0;

ADC_Cal_t adc_cal = {0};

void ADC_init(void)
{
    /* GPIO PA0 analog input first */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODE0;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;

    /* ADC clock and power */
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    ADC123_COMMON->CCR |= (1U << ADC_CCR_CKMODE_Pos);

    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    delay_us(20);

    /* ADC calibration */
    ADC1->DIFSEL &= ~ADC_DIFSEL_DIFSEL_5;
    ADC1->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL) { }

    /* Enable ADC */
    ADC1->ISR |= ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    ADC1->ISR |= ADC_ISR_ADRDY;

    /* ADC1 channel 5, 12-bit, single conversion */
    ADC1->SQR1 &= ~ADC_SQR1_L;
    ADC1->SQR1 &= ~ADC_SQR1_SQ1;
    ADC1->SQR1 |= (5U << ADC_SQR1_SQ1_Pos);

    ADC1->SMPR1 &= ~ADC_SMPR1_SMP5;
    ADC1->SMPR1 |= (2U << ADC_SMPR1_SMP5_Pos);

    ADC1->CFGR &= ~(ADC_CFGR_CONT | ADC_CFGR_EXTEN | ADC_CFGR_RES);

    /* Enable EOC interrupt */
    ADC1->IER |= ADC_IER_EOCIE;
    ADC1->ISR |= ADC_ISR_EOC;

    NVIC_EnableIRQ(ADC1_2_IRQn);
    __enable_irq();
}

uint16_t ADC_read_blocking(void)
{
    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;

    while (!adc_eoc_flag) { }

    adc_eoc_flag = 0;
    return adc_result;
}

uint16_t ADC_average_counts(uint16_t n)
{
    uint32_t sum = 0;

    for (uint16_t i = 0; i < n; i++)
    {
        sum += ADC_read_blocking();
    }

    return (uint16_t)(sum / n);
}

int32_t ADC_apply_cal(uint16_t raw)
{
    int32_t mv;

    /*
     * Regression from calibration plot:
     *
     * raw_counts = 1236.1 * voltage_V + 1.7796
     *
     * voltage_V = (raw_counts - 1.7796) / 1236.1
     *
     * Integer mV form:
     *
     * voltage_mV = (raw_counts * 10000 - 17796) / 12361
     *
     * Add 6180 for rounding because 12361 / 2 = 6180.
     */

    mv = (((int32_t)raw * 10000) - 17796 + 6180) / 12361; //from calibration graph

    if (mv < 0)
    {
        mv = 0;
    }

    if (mv > 3000)
    {
        mv = 3000;
    }

    return mv;
}
