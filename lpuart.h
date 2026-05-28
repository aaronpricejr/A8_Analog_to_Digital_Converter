#ifndef LPUART_H
#define LPUART_H

#include <stdint.h>

// FUNCTION PROTOTYPES ---------------------------------------------------------
void LPUART_init(void);
void LPUART_print(const char *message);
void LPUART_print_ADC_table(uint16_t adc_min, uint16_t adc_max, uint32_t adc_avg);

#endif /* LPUART_H */
