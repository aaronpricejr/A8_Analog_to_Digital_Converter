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
9
// 1 stop bit
LPUART1->CR2 &= ~USART_CR2_STOP;
// No parity
LPUART1->CR1 &= ~USART_CR1_PCE;
// Baud rate: 115200 with 4 MHz clock
LPUART1->BRR = (256 * 4000000) / 115200;
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
