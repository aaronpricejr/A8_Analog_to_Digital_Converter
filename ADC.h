/**
 ******************************************************************************
 * @file    : ADC.h
 * @brief   : Header for ADC module from A8 - ADC
 ******************************************************************************
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l4xx.h"
#include <stdint.h>
#include "delay.h"

#define ADC_CAL_HIGH_MV 3000
#define FULL_SCALE      4095
#define NUM_CAL_SAMPLES 256

typedef struct
{
    int32_t offset_counts;
    int32_t gain_num_mv;
    int32_t gain_den_counts;
    uint8_t valid;
} ADC_Cal_t;

extern ADC_Cal_t adc_cal;

extern volatile uint16_t adc_result;
extern volatile uint8_t adc_eoc_flag;

void ADC_init(void);
uint16_t ADC_read_blocking(void);
uint16_t ADC_average_counts(uint16_t n);
int32_t ADC_apply_cal(uint16_t raw);

#endif /* INC_ADC_H_ */
