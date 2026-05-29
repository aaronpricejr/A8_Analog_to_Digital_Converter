/**
 ******************************************************************************
 * @file    : ADC.h
 * @brief   : Header for ADC module
 * project  : EE 329 S'26 A5
 * authors  : Aaron Price Jr. & Brandon Valenti
 * version  : 0.2
 * date     : 2026-05-19
 * target   : NUCLEO-L4A6ZG
 *
 * ADC1 channel 5, PA0 (Nucleo A0)
 * 12-bit resolution, single conversion, software trigger
 * Sample time: 47.5 clock cycles (SMPR1 SMP5 = 0b100)
 * VREF = 3.3V (internal)
 ******************************************************************************
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l4xx.h"
#include <stdint.h>
#include "delay.h"

#define VREF_MV          3300
#define ADC_FULL_SCALE   4095

extern volatile uint16_t adc_result;
extern volatile uint8_t  adc_eoc_flag;

void     ADC_init(void);
uint16_t ADC_read_blocking(void);
int32_t  ADC_counts_to_mv(uint16_t raw);

#endif /* INC_ADC_H_ */
