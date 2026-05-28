/**
 ******************************************************************************
 * @file       	: ADC.h
 * @brief      	: Header for ADC module from A8 - ADC
 * 					  (Analog to Digital Converter)
 * project     	: EE 329 S'26 A5
 * authors     	: Aaron Price Jr. & Brandon Valenti
 * version     	: 0.1
 * date        	: 2026-05-19
 * compiler    	: STM32CubeIDE v.1.19.0
 * target      	: NUCLEO-L4A6ZG
 * clocks      	: 4 MHz MSI to AHB2
 ******************************************************************************
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l4xx.h"
#include <stdint.h>
#include "delay.h"

typedef struct {
    int32_t offset;
    int32_t gain_num;
    int32_t gain_den;
    uint8_t valid;
} ADC_Cal_t;

extern ADC_Cal_t adc_cal;

int32_t ADC_apply_cal(uint16_t raw);

#define VREF_MV     3300
#define FULL_SCALE  4095
extern volatile uint16_t adc_result;
extern volatile uint8_t  adc_eoc_flag;

void delay_us(uint32_t us);
void ADC_init(void);

#endif /* INC_ADC_H_ */
