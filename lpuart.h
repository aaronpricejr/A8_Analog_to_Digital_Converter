#ifndef LPUART_H
#define LPUART_H

#include <stdint.h>

void LPUART_init(void);
void LPUART_print(const char *message);
char LPUART_read_char(void);

void LPUART_clear_screen(void);

void LPUART_print_ADC_table(uint16_t min_counts,
                            uint16_t max_counts,
                            uint16_t avg_counts,
                            int32_t min_mv,
                            int32_t max_mv,
                            int32_t avg_mv);

#endif /* LPUART_H */
