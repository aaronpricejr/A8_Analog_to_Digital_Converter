/**
 ******************************************************************************
 * @file       	: main.h
 * @brief      	: Header main loop for  A8 - ADC
 * project     	: EE 329 S'26 A5
 * authors     	: Aaron Price Jr. & Brandon Valenti
 * version     	: 0.1
 * date        	: 2026-05-19
 * compiler    	: STM32CubeIDE v.1.19.0
 * target      	: NUCLEO-L4A6ZG
 * clocks      	: 4 MHz MSI to AHB2
 ******************************************************************************
 */

#ifndef INC_MAIN_H_
#define INC_MAIN_H_

#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "ADC.h"
#include "lpuart.h"
#include <stdint.h>

#define NUM_SAMPLES 20

void SystemClock_Config(void);
void Error_Handler(void);

#endif /* INC_MAIN_H_ */

