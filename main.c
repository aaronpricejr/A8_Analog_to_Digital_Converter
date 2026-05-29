/**
 ******************************************************************************
 * @file    : main.c
 * @brief   : ADC coil current monitor with relay control
 * project  : EE 329 S'26 A5
 * authors  : Aaron Price Jr. & Brandon Valenti
 * version  : 0.2
 * date     : 2026-05-19
 * compiler : STM32CubeIDE v.1.19.0
 * target   : NUCLEO-L4A6ZG
 * clocks   : 24 MHz MSI to AHB2
 *
 * ---- Circuit Description ----
 * Power:     +5VDC supply rail
 *
 * Relay:     Coil between +5VDC and Q1 collector
 *
 * DFB:       Flyback diode (1N4148) across relay coil
 *            Cathode to +5VDC, anode to Q1 collector
 *            Suppresses inductive spike when relay de-energizes
 *
 * Q1:        NPN transistor (2N2222)
 *            Base     <-- RB (1kΩ) <-- PB0 (GPIO output, active high)
 *            Collector --> relay coil --> +5VDC
 *            Emitter  --> RE (10Ω, 1%) --> GND
 *
 * RE:        10Ω sense resistor between Q1 emitter and GND
 *            ADC measures V across RE; coil current = V_RE / 10Ω
 *
 * ADC input: PA0 (Nucleo header A0)
 *            Connected to Q1 emitter / top of RE
 *            ADC1 channel 5, 12-bit, 47.5-cycle sample time
 *
 * USER PBSW: PC13, active low (Nucleo onboard button)
 *            External 4.7kΩ pull-up to 3.3V on Nucleo board
 *            Press   = PC13 low  --> PB0 high --> Q1 on  --> relay ON
 *            Release = PC13 high --> PB0 low  --> Q1 off --> relay OFF
 * --------------------------------
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

    /* ----------------------------------------------------------------
     * GPIO Init
     * ---------------------------------------------------------------- */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

    /* PB0: output, push-pull, relay OFF at startup */
    GPIOB->MODER   &= ~(0x3U << (0 * 2));
    GPIOB->MODER   |=  (0x1U << (0 * 2));
    GPIOB->OTYPER  &= ~(0x1U << 0);
    GPIOB->OSPEEDR &= ~(0x3U << (0 * 2));
    GPIOB->PUPDR   &= ~(0x3U << (0 * 2));
    GPIOB->ODR     &= ~(0x1U << 0);

    /* PC13: input mode, no pull */
    GPIOC->MODER &= ~(0x3U << (13 * 2));
    GPIOC->PUPDR &= ~(0x3U << (13 * 2));

    /* ----------------------------------------------------------------
     * Main Loop
     *
     * Each iteration:
     *   1. Poll USER button and drive relay
     *   2. Collect ADC samples
     *   3. Compute min/avg/max and print results
     * ---------------------------------------------------------------- */
    uint16_t samples[NUM_SAMPLES];
    uint8_t  sample_idx = 0;

    uint32_t adc_sum = 0;

    uint16_t adc_min_raw;
    uint16_t adc_max_raw;
    uint16_t adc_avg_raw;

    int32_t adc_min_mv;
    int32_t adc_avg_mv;
    int32_t adc_max_mv;

    /* Start first conversion */
    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;

    while (1)
    {
        /* ------------------------------------------------------------
         * Relay Control
         * PC13 is active low:
         *   pressed   -> 0
         *   released  -> 1
         * ------------------------------------------------------------ */
        if (!(GPIOC->IDR & (1U << 13)))
        {
            GPIOB->ODR |= (1U << 0);     /* relay ON */
        }
        else
        {
            GPIOB->ODR &= ~(1U << 0);    /* relay OFF */
        }

        /* ------------------------------------------------------------
         * ADC Sample Collection
         * ------------------------------------------------------------ */
        if (adc_eoc_flag)
        {
            samples[sample_idx++] = adc_result;

            adc_eoc_flag = 0;

            /* Start next conversion */
            ADC1->CR |= ADC_CR_ADSTART;

            if (sample_idx >= NUM_SAMPLES)
            {
                sample_idx = 0;

                adc_min_raw = 0xFFFF;
                adc_max_raw = 0x0000;
                adc_sum     = 0;

                for (uint8_t i = 0; i < NUM_SAMPLES; i++)
                {
                    uint16_t s = samples[i];

                    if (s < adc_min_raw)
                    {
                        adc_min_raw = s;
                    }

                    if (s > adc_max_raw)
                    {
                        adc_max_raw = s;
                    }

                    adc_sum += s;
                }

                adc_avg_raw = (uint16_t)(adc_sum / NUM_SAMPLES);

                /* Convert raw counts to millivolts */
                adc_min_mv = ADC_counts_to_mv(adc_min_raw);
                adc_avg_mv = ADC_counts_to_mv(adc_avg_raw);
                adc_max_mv = ADC_counts_to_mv(adc_max_raw);

                /* Print ADC table */
                LPUART_print_ADC_table(
                    adc_min_raw,
                    adc_max_raw,
                    adc_avg_raw,
                    adc_min_mv,
                    adc_max_mv,
                    adc_avg_mv
                );

                /* ----------------------------------------------------
                 * Coil Current Estimate
                 * I = V / R
                 * ---------------------------------------------------- */
                int32_t coil_mA = adc_avg_mv / RE_OHMS;

                int32_t whole = coil_mA / 1000;
                int32_t frac  = coil_mA % 1000;

                char cur_str[64];
                uint8_t ci = 0;

                for (uint8_t sp = 0; sp < 29; sp++)
                {
                    cur_str[ci++] = ' ';
                }

                const char *label = "coil current = ";

                uint8_t li = 0;

                while (label[li])
                {
                    cur_str[ci++] = label[li++];
                }

                cur_str[ci++] = (char)('0' + whole);
                cur_str[ci++] = '.';
                cur_str[ci++] = (char)('0' + (frac / 100));
                cur_str[ci++] = (char)('0' + ((frac / 10) % 10));
                cur_str[ci++] = (char)('0' + (frac % 10));

                cur_str[ci++] = ' ';
                cur_str[ci++] = 'A';
                cur_str[ci++] = '\r';
                cur_str[ci++] = '\n';

                cur_str[ci] = '\0';

                LPUART_print(cur_str);

                delay_ms(500);
            }
        }
    }
}

/* ----------------------------------------------------------------
 * ADC1 End-of-Conversion ISR
 * ---------------------------------------------------------------- */
void ADC1_2_IRQHandler(void)
{
    if (ADC1->ISR & ADC_ISR_EOC)
    {
        adc_result = (uint16_t)(ADC1->DR & 0x0FFF);
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

    RCC_ClkInitStruct.ClockType      =
        RCC_CLOCKTYPE_HCLK  |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct,
                            FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
