/**
******************************************************************************
* @file : lpuart.c
* @brief : Main program body
*
******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx.h"
#include "lpuart.h"

void LPUART_init(void) {
// Enable VDDIO2 for GPIOG pins PG[15:2]
PWR->CR2 |= PWR_CR2_IOSV;
// Enable GPIOG clock
RCC->AHB2ENR |= RCC_AHB2ENR_GPIOGEN;
// Enable LPUART1 clock
RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
// PG7 = LPUART1_TX, PG8 = LPUART1_RX, AF8
// Set PG7 and PG8 to alternate function mode: MODER = 10
GPIOG->MODER &= ~((3U << (7 * 2)) | (3U << (8 * 2)));
GPIOG->MODER |= ((2U << (7 * 2)) | (2U << (8 * 2)));
// Output type: push-pull
GPIOG->OTYPER &= ~((1U << 7) | (1U << 8));
// High speed
GPIOG->OSPEEDR &= ~((3U << (7 * 2)) | (3U << (8 * 2)));
GPIOG->OSPEEDR |= ((3U << (7 * 2)) | (3U << (8 * 2)));
// Pull-up resistors
GPIOG->PUPDR &= ~((3U << (7 * 2)) | (3U << (8 * 2)));
GPIOG->PUPDR |= ((1U << (7 * 2)) | (1U << (8 * 2)));
// Select AF8 for PG7 and PG8
// PG7 is in AFR[0]
GPIOG->AFR[0] &= ~(0xFU << (7 * 4));
GPIOG->AFR[0] |= (8U << (7 * 4));
// PG8 is in AFR[1], position 0
GPIOG->AFR[1] &= ~(0xFU << ((8 - 8) * 4));
GPIOG->AFR[1] |= (8U << ((8 - 8) * 4));
// Disable LPUART before configuration
LPUART1->CR1 &= ~USART_CR1_UE;
// 8-bit data
LPUART1->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0);

// 1 stop bit
LPUART1->CR2 &= ~USART_CR2_STOP;
// No parity
LPUART1->CR1 &= ~USART_CR1_PCE;
// Baud rate: 115200 with 4 MHz clock
LPUART1->BRR = 0xD056;
// Enable transmitter and receiver
LPUART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
// Enable LPUART
LPUART1->CR1 |= USART_CR1_UE;
}

void LPUART_print(const char *message) {
uint16_t i = 0;
while (message[i] != '\0') {
while (!(LPUART1->ISR & USART_ISR_TXE))
;
LPUART1->TDR = message[i];
i++;
}
}

static void LPUART_print_char(char c)
{
    while (!(LPUART1->ISR & USART_ISR_TXE)) { ; }
    LPUART1->TDR = c;
}

static void LPUART_print_uint_width(uint32_t value, uint8_t width)
{
    char buf[10];
    uint8_t len = 0;

    if (value == 0)
    {
        buf[len++] = '0';
    }
    else
    {
        uint32_t v = value;
        while (v > 0)
        {
            buf[len++] = '0' + (v % 10);
            v /= 10;
        }
    }

    for (uint8_t i = len; i < width; i++)
        LPUART_print_char(' ');

    for (int8_t i = len - 1; i >= 0; i--)
        LPUART_print_char(buf[i]);
}

static void LPUART_print_voltage(uint16_t count)
{
    uint32_t mv    = ((uint32_t)count * 3300UL) / 4095UL;
    uint32_t whole = mv / 1000;
    uint32_t frac  = mv % 1000;

    LPUART_print_char(' ');
    LPUART_print_char('0' + (char)whole);
    LPUART_print_char('.');
    LPUART_print_char('0' + (char)(frac / 100));
    LPUART_print_char('0' + (char)((frac / 10) % 10));
    LPUART_print_char('0' + (char)(frac % 10));
    LPUART_print_char(' ');
    LPUART_print_char('V');
}

static void LPUART_print_row(const char *label, uint16_t count)
{
    LPUART_print(label);
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_uint_width((uint32_t)count, 4);
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_voltage(count);
    LPUART_print_char('\r');
    LPUART_print_char('\n');
}

void LPUART_print_ADC_table(uint16_t adc_min, uint16_t adc_max, uint32_t adc_avg)
{
    LPUART_print("\033[H");   // move cursor to top-left, do NOT clear screen

    LPUART_print("ADC  counts  volts\r\n");

    LPUART_print_row("MIN", adc_min);
    LPUART_print_row("MAX", adc_max);
    LPUART_print_row("AVG", (uint16_t)adc_avg);
}

