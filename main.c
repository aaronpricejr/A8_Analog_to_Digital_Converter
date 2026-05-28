/**
 ******************************************************************************
 * @file       	: main.c
 * @brief      	: Main loop, and interrupt handling ADC module from A8 - ADC
 * 				  (Analog to Digital Converter)
 * project     	: EE 329 S'26 A5
 * authors     	: Aaron Price Jr. & Brandon Valenti
 * version     	: 0.1
 * date        	: 2026-05-19
 * compiler    	: STM32CubeIDE v.1.19.0
 * target      	: NUCLEO-L4A6ZG
 * clocks      	: 4 MHz MSI to AHB2
 ******************************************************************************
 */

#include "main.h"

void SystemClock_Config(void);

/**
 ******************************************************************************
 * @file       	: main.c
 * @brief      	: Main loop, and interrupt handling ADC module from A8 - ADC
 * 				  (Analog to Digital Converter)
 * project     	: EE 329 S'26 A5
 * authors     	: Aaron Price Jr. & Brandon Valenti
 * version     	: 0.1
 * date        	: 2026-05-19
 * compiler    	: STM32CubeIDE v.1.19.0
 * target      	: NUCLEO-L4A6ZG
 * clocks      	: 4 MHz MSI to AHB2
 ******************************************************************************
 */

#include "main.h"

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    SysTick_Init();
    LPUART_init();
    ADC_init();

    // ---- Two-Point Calibration ----
    uint32_t cal_sum;
    int32_t  code_low, code_high;

    // -- Low point: apply 0V --
    LPUART_print_str("Apply 0V to ADC input, press Enter...\r\n");
    while (LPUART_read_char() != '\r'); // Block until Enter

    cal_sum = 0;
    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;
    for (uint16_t i = 0; i < 256; i++)
    {
        while (!adc_eoc_flag);
        cal_sum += adc_result;
        adc_eoc_flag = 0;
        ADC1->CR |= ADC_CR_ADSTART;
    }
    code_low = (int32_t)(cal_sum / 256);

    // -- High point: apply 3.3V --
    LPUART_print_str("Apply 3.3V to ADC input, press Enter...\r\n");
    while (LPUART_read_char() != '\r'); // Block until Enter

    cal_sum = 0;
    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;
    for (uint16_t i = 0; i < 256; i++)
    {
        while (!adc_eoc_flag);
        cal_sum += adc_result;
        adc_eoc_flag = 0;
        ADC1->CR |= ADC_CR_ADSTART;
    }
    code_high = (int32_t)(cal_sum / 256);

    // -- Store coefficients --
    if ((code_high - code_low) >= 100)
    {
        adc_cal.offset   = code_low;
        adc_cal.gain_num = 3300;                      // VREF in mV
        adc_cal.gain_den = code_high - code_low;
        adc_cal.valid    = 1;
        LPUART_print_str("Calibration OK!\r\n");
    }
    else
    {
        LPUART_print_str("Calibration FAILED - check wiring!\r\n");
    }

    // ---- Main Loop ----
    uint16_t samples[NUM_SAMPLES];
    uint8_t  sample_idx = 0;
    uint32_t adc_sum    = 0;
    uint16_t adc_min_raw, adc_max_raw;
    int32_t  adc_min_mv, adc_avg_mv, adc_max_mv;

    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;

    while (1)
    {
        if (adc_eoc_flag)
        {
            samples[sample_idx++] = adc_result;
            adc_eoc_flag = 0;
            ADC1->CR |= ADC_CR_ADSTART;

            if (sample_idx >= NUM_SAMPLES)
            {
                sample_idx  = 0;
                adc_min_raw = 0xFFFF;
                adc_max_raw = 0x0000;
                adc_sum     = 0;

                for (uint8_t i = 0; i < NUM_SAMPLES; i++)
                {
                    uint16_t s = samples[i];
                    if (s < adc_min_raw) adc_min_raw = s;
                    if (s > adc_max_raw) adc_max_raw = s;
                    adc_sum += s;
                }

                adc_min_mv = ADC_apply_cal(adc_min_raw);
                adc_avg_mv = ADC_apply_cal((uint16_t)(adc_sum / NUM_SAMPLES));
                adc_max_mv = ADC_apply_cal(adc_max_raw);

                LPUART_print_ADC_table(adc_min_mv, adc_avg_mv, adc_max_mv);
                delay_ms(500);
            }
        }
    }
}

void ADC1_2_IRQHandler(void)
{
    if (ADC1->ISR & ADC_ISR_EOC)
    {
        adc_result   = (uint16_t)(ADC1->DR & 0x0FFF);
        adc_eoc_flag = 1;
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_9;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif
