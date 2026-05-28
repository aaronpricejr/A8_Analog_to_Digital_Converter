/**
 ******************************************************************************
 * @file    : main.h
 * @brief   : Header for main.c
 * project  : EE 329 S'26 A5
 * authors  : Aaron Price Jr. & Brandon Valenti
 * version  : 0.2
 * date     : 2026-05-19
 * compiler : STM32CubeIDE v.1.19.0
 * target   : NUCLEO-L4A6ZG
 * clocks   : 24 MHz MSI to AHB2
 ******************************************************************************
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_

#include "stm32l4xx_hal.h"
#include "ADC.h"
#include "lpuart.h"
#include "delay.h"

/* Number of ADC samples to collect per display update */
#define NUM_SAMPLES  16

/*
 * RE = 10Ω sense resistor on Q1 emitter (see schematic)
 * Coil current (mA) = V_RE (mV) / RE_OHMS
 */
#define RE_OHMS  10

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* INC_MAIN_H_ */
