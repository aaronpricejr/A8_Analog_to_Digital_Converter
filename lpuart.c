/**
 ******************************************************************************
 * @file    : lpuart.c
 * @brief   : LPUART1 initialization and ADC table display
 ******************************************************************************
 */

#include "stm32l4xx.h"
#include "lpuart.h"

void LPUART_init(void)
{
    /* Enable VDDIO2 for GPIOG pins PG[15:2] */
    PWR->CR2 |= PWR_CR2_IOSV;

    /* Enable GPIOG clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOGEN;

    /* Enable LPUART1 clock */
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    /*
     * PG7 = LPUART1_TX
     * PG8 = LPUART1_RX
     * AF8 = LPUART1
     */

    GPIOG->MODER &= ~((3U << (7 * 2)) | (3U << (8 * 2)));
    GPIOG->MODER |=  ((2U << (7 * 2)) | (2U << (8 * 2)));

    GPIOG->OTYPER &= ~((1U << 7) | (1U << 8));

    GPIOG->OSPEEDR &= ~((3U << (7 * 2)) | (3U << (8 * 2)));
    GPIOG->OSPEEDR |=  ((3U << (7 * 2)) | (3U << (8 * 2)));

    GPIOG->PUPDR &= ~((3U << (7 * 2)) | (3U << (8 * 2)));
    GPIOG->PUPDR |=  ((1U << (7 * 2)) | (1U << (8 * 2)));

    GPIOG->AFR[0] &= ~(0xFU << (7 * 4));
    GPIOG->AFR[0] |=  (8U << (7 * 4));

    GPIOG->AFR[1] &= ~(0xFU << ((8 - 8) * 4));
    GPIOG->AFR[1] |=  (8U << ((8 - 8) * 4));

    LPUART1->CR1 &= ~USART_CR1_UE;

    LPUART1->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0);
    LPUART1->CR2 &= ~USART_CR2_STOP;
    LPUART1->CR1 &= ~USART_CR1_PCE;

    /* 115200 baud with 4 MHz clock */
    LPUART1->BRR = 0xD056;

    LPUART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    LPUART1->CR1 |= USART_CR1_UE;
}

static void LPUART_print_char(char c)
{
    while (!(LPUART1->ISR & USART_ISR_TXE)) { }
    LPUART1->TDR = c;
}

void LPUART_print(const char *message)
{
    uint16_t i = 0;

    while (message[i] != '\0')
    {
        LPUART_print_char(message[i]);
        i++;
    }
}

char LPUART_read_char(void)
{
    char c;

    while (!(LPUART1->ISR & USART_ISR_RXNE)) { }

    c = (char)(LPUART1->RDR & 0xFF);

    /*
     * Treat newline as Enter too.
     * Some terminals send '\n', others send '\r'.
     */
    if (c == '\n')
    {
        c = '\r';
    }

    return c;
}

void LPUART_clear_screen(void)
{
    LPUART_print("\033[2J");   /* clear entire terminal */
    LPUART_print("\033[H");    /* move cursor to top-left */
}

static void LPUART_cursor_home(void)
{
    LPUART_print("\033[H");
}

static void LPUART_print_4digit_counts(uint16_t value)
{
    uint16_t divisor;
    uint8_t digit;

    if (value > 9999)
    {
        value = 9999;
    }

    divisor = 1000;

    while (divisor > 0)
    {
        digit = value / divisor;
        LPUART_print_char((char)('0' + digit));
        value = value % divisor;
        divisor = divisor / 10;
    }
}

static void LPUART_print_voltage_mv(int32_t mv)
{
    uint32_t whole;
    uint32_t frac;

    if (mv < 0)
    {
        mv = 0;
    }

    if (mv > 3000)
    {
        mv = 3000;
    }

    whole = (uint32_t)mv / 1000;
    frac  = (uint32_t)mv % 1000;

    LPUART_print_char((char)('0' + whole));
    LPUART_print_char('.');
    LPUART_print_char((char)('0' + (frac / 100)));
    LPUART_print_char((char)('0' + ((frac / 10) % 10)));
    LPUART_print_char((char)('0' + (frac % 10)));
    LPUART_print(" V");
}

static void LPUART_print_ADC_row(const char *label,
                                 uint16_t counts,
                                 int32_t mv)
{
    LPUART_print(label);
    LPUART_print("  ");
    LPUART_print_4digit_counts(counts);
    LPUART_print("  ");
    LPUART_print_voltage_mv(mv);
    LPUART_print("     ");
    LPUART_print("\r\n");
}

void LPUART_print_ADC_table(uint16_t min_counts,
                            uint16_t max_counts,
                            uint16_t avg_counts,
                            int32_t min_mv,
                            int32_t max_mv,
                            int32_t avg_mv)
{
    LPUART_cursor_home();

    LPUART_print("ADC counts volts\r\n");

    LPUART_print_ADC_row("MIN", min_counts, min_mv);
    LPUART_print_ADC_row("MAX", max_counts, max_mv);
    LPUART_print_ADC_row("AVG", avg_counts, avg_mv);
}
