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
     *
     * PB0:  Output, push-pull, no pull
     *       Drives RB (1kΩ) --> Q1 base --> relay coil
     *       HIGH = relay ON, LOW = relay OFF
     *
     * PC13: Input, no pull
     *       USER pushbutton, active low
     *       Nucleo board has external 4.7kΩ pull-up to 3.3V
     * ---------------------------------------------------------------- */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;

    /* PB0: output, push-pull, low speed, no pull, start LOW (relay OFF) */
    GPIOB->MODER   &= ~(0x3U << (0 * 2));
    GPIOB->MODER   |=  (0x1U << (0 * 2));
    GPIOB->OTYPER  &= ~(0x1U << 0);
    GPIOB->OSPEEDR &= ~(0x3U << (0 * 2));
    GPIOB->PUPDR   &= ~(0x3U << (0 * 2));
    GPIOB->ODR     &= ~(0x1U << 0);            /* relay OFF on startup */

    /* PC13: input mode (00), no pull (board provides external pull-up) */
    GPIOC->MODER   &= ~(0x3U << (13 * 2));
    GPIOC->PUPDR   &= ~(0x3U << (13 * 2));

    /* ----------------------------------------------------------------
     * Two-Point ADC Calibration
     *
     * Apply known voltages to PA0 (A0) when prompted via terminal.
     * 256 samples are averaged at each point to reduce noise.
     *
     * Low  point: connect PA0 to GND     (0V)
     * High point: connect PA0 to 3.3V pin on Nucleo
     * ---------------------------------------------------------------- */
    int32_t code_low, code_high;

    LPUART_clear_screen();

    /* Low point: short PA0 to GND */
    LPUART_print("=== ADC Two-Point Calibration ===\r\n");
    LPUART_print("Apply 0V (GND) to PA0 (A0), press Enter...\r\n");
    while (LPUART_read_char() != '\r');
    code_low = (int32_t)ADC_average_counts(NUM_CAL_SAMPLES);

    /* High point: connect PA0 to 3.3V pin */
    LPUART_print("Apply 3.3V to PA0 (A0), press Enter...\r\n");
    while (LPUART_read_char() != '\r');
    code_high = (int32_t)ADC_average_counts(NUM_CAL_SAMPLES);

    /* Store coefficients if spread is large enough to be valid */
    if ((code_high - code_low) >= 100)
    {
        adc_cal.offset_counts    = code_low;
        adc_cal.gain_num_mv      = 3300;              /* VREF = 3.3V = 3300mV */
        adc_cal.gain_den_counts  = code_high - code_low;
        adc_cal.valid            = 1;
        LPUART_print("Calibration OK!\r\n");
    }
    else
    {
        /* Calibration failed - ADC will use uncalibrated fallback */
        adc_cal.valid = 0;
        LPUART_print("Calibration FAILED - check wiring!\r\n");
    }

    LPUART_clear_screen();

    /* ----------------------------------------------------------------
     * Main Loop
     *
     * Each iteration:
     *   1. Poll USER button (PC13) and drive relay (PB0) accordingly
     *   2. Collect ADC samples into circular buffer
     *   3. Every NUM_SAMPLES conversions: compute min/avg/max,
     *      apply calibration, print table and coil current estimate
     * ---------------------------------------------------------------- */
    uint16_t samples[NUM_SAMPLES];
    uint8_t  sample_idx = 0;
    uint32_t adc_sum    = 0;
    uint16_t adc_min_raw, adc_max_raw, adc_avg_raw;
    int32_t  adc_min_mv, adc_avg_mv, adc_max_mv;

    /* Kick off first conversion */
    adc_eoc_flag = 0;
    ADC1->CR |= ADC_CR_ADSTART;

    while (1)
    {
        /* -- Relay control --
         * PC13 reads 0 when USER button is pressed (active low)
         * PB0 high saturates Q1 and energizes relay coil           */
        if ((GPIOC->IDR & (1U << 13)))
            GPIOB->ODR |=  (1U << 0);   /* button pressed  -> relay ON  */
        else
            GPIOB->ODR &= ~(1U << 0);   /* button released -> relay OFF */

        /* -- ADC sample collection -- */
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

                adc_avg_raw = (uint16_t)(adc_sum / NUM_SAMPLES);
                adc_min_mv  = ADC_apply_cal(adc_min_raw);
                adc_avg_mv  = ADC_apply_cal(adc_avg_raw);
                adc_max_mv  = ADC_apply_cal(adc_max_raw);

                /* Print ADC table: counts and calibrated voltage */
                LPUART_print_ADC_table(adc_min_raw, adc_max_raw, adc_avg_raw,
                                       adc_min_mv,  adc_max_mv,  adc_avg_mv);

                /* -- Coil current estimate (integer math only) --
                 * V_RE  (mV) = adc_avg_mv  (voltage across 10Ω sense resistor)
                 * I_coil(mA) = V_RE (mV) / RE (Ω) = adc_avg_mv / 10
                 * Format: "coil current = X.XXX A\r\n"
                 * Uses char buffer + LPUART_print() — no float, no printf    */
                int32_t coil_mA = adc_avg_mv / RE_OHMS;
                int32_t whole   = coil_mA / 1000;   /* amps, whole part      */
                int32_t frac    = coil_mA % 1000;   /* milliamps, frac part  */

                char cur_str[32];
                uint8_t ci = 0;

                const char *label = "coil current = ";
                while (label[ci]) { cur_str[ci] = label[ci]; ci++; }

                cur_str[ci++] = (char)('0' + whole);
                cur_str[ci++] = '.';
                cur_str[ci++] = (char)('0' + (frac / 100));
                cur_str[ci++] = (char)('0' + ((frac / 10) % 10));
                cur_str[ci++] = (char)('0' + (frac % 10));
                cur_str[ci++] = ' ';
                cur_str[ci++] = 'A';
                cur_str[ci++] = '\r';
                cur_str[ci++] = '\n';
                cur_str[ci]   = '\0';

                LPUART_print(cur_str);

                delay_ms(500);
            }
        }
    }
}

/* ----------------------------------------------------------------
 * ADC1 End-of-Conversion ISR
 * Fires after each single conversion triggered by ADC_CR_ADSTART
 * Stores result and sets flag for main loop to consume
 * ---------------------------------------------------------------- */
void ADC1_2_IRQHandler(void)
{
    if (ADC1->ISR & ADC_ISR_EOC)
    {
        adc_result   = (uint16_t)(ADC1->DR & 0x0FFF); /* 12-bit result */
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
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_9; /* 24 MHz */
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
