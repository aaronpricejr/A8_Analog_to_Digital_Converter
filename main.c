/**
 ******************************************************************************
 * @file       	: main.c
 * @brief      	: Main loop, and interupt handlng ADC module from A8 - ADC
 * 					  (Analog to Digital Converter)
 * project     	: EE 329 S'26 A5
 * authors     	: Aaron Price Jr. & Brandon Valenti
 * version     	: 0.1
 * date        	: 2026-05-19
 * compiler    	: STM32CubeIDE v.1.19.0
 * target      	: NUCLEO-L4A6ZG
 * clocks      	: 4 MHz MSI to AHB2
 *
 * +5V -> Relay Pin 1
 * +3.3V -> Base of BJT
 * PA0 (ADC) -> Emitter of BJT
 *
 ******************************************************************************
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_
#endif /* INC_ADC_H_ */

#include "main.h"

void SystemClock_Config(void);

int main(void)
{
    /* Configure SYSCLK >= 24 MHz here (PLL setup) */

    LPUART_init();
    ADC_init();

    uint16_t samples[NUM_SAMPLES];
    uint8_t  sample_idx = 0;

    //INITIALIZE ALL ADC variables
    uint16_t ADC_Min = 0xFFFF;
    uint16_t ADC_Max = 0x0000;
    uint32_t ADC_Sum = 0;
    uint32_t ADC_Avg = 0;

    while (1)
    {
        if (adc_eoc_flag)
        {
            samples[sample_idx] = adc_result;
            sample_idx++;

            adc_eoc_flag = 0;							// Reset interupt flag
            ADC1->CR |= ADC_CR_ADSTART;         // start next conversion

            if (sample_idx >= NUM_SAMPLES)
            {
                sample_idx = 0;

                ADC_Min = 0xFFFF;
                ADC_Max = 0x0000;
                ADC_Sum = 0;

                // Take ADC value across N samples, and obtain average
                for (uint8_t i = 0; i < NUM_SAMPLES; i++)
                {
                    uint16_t s = samples[i];
                    if (s < ADC_Min) ADC_Min = s;
                    if (s > ADC_Max) ADC_Max = s;
                    ADC_Sum += s;
                }

                ADC_Avg = ADC_Sum / NUM_SAMPLES;

                // Print the updated table to the terminal
                LPUART_print_ADC_table(ADC_Min, ADC_Max, ADC_Avg);
            }
        }
    }
}

void ADC1_2_IRQHandler(void) {
	if (ADC1->ISR & ADC_ISR_EOC) {
		ADC_Result = (uint16t)(ADC1->DR & 0x0FFF);	// Copy 12 bit ADC result
		ADC_EOC_Flag = 1;										// new sample available for main loop
	}

}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}


void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
