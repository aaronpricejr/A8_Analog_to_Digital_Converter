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

#define VREF_MV          3300   /* ADC reference voltage in millivolts        */
#define ADC_FULL_SCALE   4095   /* 12-bit max count                           */
#define NUM_CAL_SAMPLES   256   /* samples averaged per calibration point      */

/*
 * Two-point calibration coefficients.
 * Populated at startup by measuring 0V and 3.3V on PA0.
 * Used by ADC_apply_cal() to convert raw counts to millivolts.
 */
typedef struct
{
    int32_t offset_counts;      /* ADC count at 0V input                      */
    int32_t gain_num_mv;        /* gain numerator   = VREF in mV (3300)       */
    int32_t gain_den_counts;    /* gain denominator = code_high - code_low     */
    uint8_t valid;              /* 1 = calibration complete, 0 = uncalibrated  */
} ADC_Cal_t;

extern ADC_Cal_t         adc_cal;      /* calibration coefficients             */
extern volatile uint16_t adc_result;   /* latest raw ADC count from ISR        */
extern volatile uint8_t  adc_eoc_flag; /* set by ISR, cleared by main loop     */

void     ADC_init(void);
uint16_t ADC_read_blocking(void);
uint16_t ADC_average_counts(uint16_t n);
int32_t  ADC_apply_cal(uint16_t raw);

#endif /* INC_ADC_H_ */
