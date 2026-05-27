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

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
