/**
 ******************************************************************************
 * @file    : lpuart.c
 * @brief   : LPUART1 init, terminal display, centered bar graph and table
 *
 * Terminal: 80x25 columns. All content is 31 chars wide, centered with a
 * 24-space left pad: (80 - 31) / 2 = 24.
 *
 * Box drawing uses CP437 extended ASCII (compatible with PuTTY, CoolTerm).
 ******************************************************************************
 */

#include "stm32l4xx.h"
#include "lpuart.h"

/* ---- Layout ---- */
#define CENTER_PAD   24   /* left pad to center 31-char content in 80 cols   */
#define BAR_SCALE   100   /* millivolts per '#' hash character                */
#define BAR_MAX      30   /* max hashes: 30 x 100mV = 3000mV full scale       */

/* ---- CP437 box-drawing characters ----
 * These render as single-line box borders in PuTTY / CoolTerm
 * with CP437 (DOS) encoding selected in terminal settings.       */
#define TL  '+'   /* top-left corner     */
#define TR  '+'   /* top-right corner    */
#define BL  '+'   /* bottom-left corner  */
#define BR  '+'   /* bottom-right corner */
#define HL  '-'   /* horizontal line     */
#define VL  '|'   /* vertical line       */
#define LT  '+'   /* left tee            */
#define RT  '+'   /* right tee           */
#define TT  '+'   /* top tee             */
#define BT  '+'   /* bottom tee          */
#define CR  '+'   /* cross               */

/* ======================================================
 * Private helpers
 * ====================================================== */

static void LPUART_print_char(char c)
{
    while (!(LPUART1->ISR & USART_ISR_TXE)) { }
    LPUART1->TDR = c;
}

static void LPUART_cursor_home(void)
{
    LPUART_print("\033[H");
}

/* Print n space characters */
static void LPUART_print_spaces(uint8_t n)
{
    for (uint8_t i = 0; i < n; i++) LPUART_print_char(' ');
}

/*
 * LPUART_print_4digit_counts
 * Prints a uint16_t value as exactly 4 zero-padded decimal digits.
 * Clamps to 9999 if value exceeds range.
 * Example: 382 -> "0382"
 */
static void LPUART_print_4digit_counts(uint16_t value)
{
    if (value > 9999) value = 9999;
    uint16_t divisor = 1000;
    while (divisor > 0)
    {
        LPUART_print_char((char)('0' + (value / divisor)));
        value   = value % divisor;
        divisor = divisor / 10;
    }
}

/*
 * LPUART_print_voltage_mv
 * Prints a millivolt value as exactly 7 characters: "X.XXX V"
 * Range: 0.000 V to 3.300 V (clamped).
 * Example: 307 mV -> "0.307 V"
 */
static void LPUART_print_voltage_mv(int32_t mv)
{
    if (mv < 0)    mv = 0;
    if (mv > 3300) mv = 3300;   /* VREF = 3.3V */

    uint32_t whole = (uint32_t)mv / 1000;
    uint32_t frac  = (uint32_t)mv % 1000;

    LPUART_print_char((char)('0' + whole));
    LPUART_print_char('.');
    LPUART_print_char((char)('0' + (frac / 100)));
    LPUART_print_char((char)('0' + ((frac / 10) % 10)));
    LPUART_print_char((char)('0' + (frac % 10)));
    LPUART_print(" V");
}

/*
 * LPUART_print_hline
 * Prints one full-width horizontal border line of the table (centered).
 *
 * left:  left corner/tee character  (TL / LT / BL)
 * junc:  interior junction character (TT / CR / BT)
 * right: right corner/tee character (TR / RT / BR)
 *
 * Column inner widths: 5, 8, 14  -> total table width = 31 chars
 *
 * Top: ┌─────┬────────┬──────────────┐
 * Mid: ├─────┼────────┼──────────────┤
 * Bot: └─────┴────────┴──────────────┘
 */
static void LPUART_print_hline(char left, char junc, char right)
{
    LPUART_print_spaces(CENTER_PAD);
    LPUART_print_char(left);
    for (uint8_t i = 0; i < 5;  i++) LPUART_print_char(HL); /* col 1: 5  */
    LPUART_print_char(junc);
    for (uint8_t i = 0; i < 8;  i++) LPUART_print_char(HL); /* col 2: 8  */
    LPUART_print_char(junc);
    for (uint8_t i = 0; i < 14; i++) LPUART_print_char(HL); /* col 3: 14 */
    LPUART_print_char(right);
    LPUART_print("\r\n");
}

/*
 * LPUART_print_table_row
 * Prints one centered data row inside the box-drawn table.
 *
 * label:  3-char string "MIN", "MAX", or "AVG"
 * counts: raw 12-bit ADC count  -> printed as 4 zero-padded digits
 * mv:     calibrated millivolts -> printed as "X.XXX V" (7 chars)
 *
 * Column inner widths (5 | 8 | 14):
 *   │ MIN │  0382  │   0.307 V    │
 */
static void LPUART_print_table_row(const char *label, uint16_t counts, int32_t mv)
{
    LPUART_print_spaces(CENTER_PAD);
    LPUART_print_char(VL);
    LPUART_print_char(' ');
    LPUART_print(label);                    /* 3 chars                     */
    LPUART_print_char(' ');                 /* col 1 total: 5 chars        */
    LPUART_print_char(VL);
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_4digit_counts(counts);     /* 4 chars                     */
    LPUART_print_char(' ');
    LPUART_print_char(' ');                 /* col 2 total: 8 chars        */
    LPUART_print_char(VL);
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_voltage_mv(mv);            /* 7 chars: "X.XXX V"          */
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_char(' ');
    LPUART_print_char(' ');                 /* col 3 total: 14 chars       */
    LPUART_print_char(VL);
    LPUART_print("\r\n");
}

/*
 * LPUART_print_bar_graph
 * Prints a horizontal '#' bar graph of avg_mv on a 0–3.0 V scale.
 * Each '#' = 100 mV (BAR_SCALE). Maximum 30 hashes (BAR_MAX).
 *
 * Example output for avg_mv = 307 mV (with 24-space center pad):
 *                         ###
 *                         |----|----|----|----|----|----|
 *                         0    0.5  1.0  1.5  2.0  2.5  3.0
 */
static void LPUART_print_bar_graph(int32_t avg_mv)
{
    if (avg_mv < 0)    avg_mv = 0;
    if (avg_mv > 3000) avg_mv = 3000;

    uint8_t num_hashes = (uint8_t)(avg_mv / BAR_SCALE);   /* 0 to 30 */

    /* Hash bar line */
    LPUART_print_spaces(CENTER_PAD);
    for (uint8_t i = 0; i < num_hashes; i++) LPUART_print_char('#');
    LPUART_print("\r\n");

    /* Scale line: 7 tick marks, 6 segments of 4 dashes = 31 chars total  */
    LPUART_print_spaces(CENTER_PAD);
    LPUART_print("|----|----|----|----|----|----|");
    LPUART_print("\r\n");

    /* Label line: tick values at 0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0 V     */
    LPUART_print_spaces(CENTER_PAD);
    LPUART_print("0    0.5  1.0  1.5  2.0  2.5  3.0");
    LPUART_print("\r\n");
}

/* ======================================================
 * Public functions
 * ====================================================== */

void LPUART_init(void)
{
    PWR->CR2  |=  PWR_CR2_IOSV;
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOGEN;
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;

    GPIOG->MODER   &= ~((3U << (7*2)) | (3U << (8*2)));
    GPIOG->MODER   |=  ((2U << (7*2)) | (2U << (8*2)));
    GPIOG->OTYPER  &= ~((1U << 7) | (1U << 8));
    GPIOG->OSPEEDR &= ~((3U << (7*2)) | (3U << (8*2)));
    GPIOG->OSPEEDR |=  ((3U << (7*2)) | (3U << (8*2)));
    GPIOG->PUPDR   &= ~((3U << (7*2)) | (3U << (8*2)));
    GPIOG->PUPDR   |=  ((1U << (7*2)) | (1U << (8*2)));
    GPIOG->AFR[0]  &= ~(0xFU << (7*4));
    GPIOG->AFR[0]  |=  (8U   << (7*4));
    GPIOG->AFR[1]  &= ~(0xFU << ((8-8)*4));
    GPIOG->AFR[1]  |=  (8U   << ((8-8)*4));

    LPUART1->CR1 &= ~USART_CR1_UE;
    LPUART1->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0);
    LPUART1->CR2 &= ~USART_CR2_STOP;
    LPUART1->CR1 &= ~USART_CR1_PCE;
    LPUART1->BRR  =  0xD056;    /* 115200 baud at 4 MHz clock */
    LPUART1->CR1 |=  USART_CR1_TE | USART_CR1_RE;
    LPUART1->CR1 |=  USART_CR1_UE;
}

void LPUART_print(const char *message)
{
    uint16_t i = 0;
    while (message[i] != '\0') { LPUART_print_char(message[i]); i++; }
}

char LPUART_read_char(void)
{
    char c;
    while (!(LPUART1->ISR & USART_ISR_RXNE)) { }
    c = (char)(LPUART1->RDR & 0xFF);
    if (c == '\n') c = '\r';
    return c;
}

void LPUART_clear_screen(void)
{
    LPUART_print("\033[2J");
    LPUART_print("\033[H");
}

/*
 * LPUART_print_ADC_table
 * Homes cursor and redraws the full live display each call.
 *
 * Layout (centered in 80x25 terminal):
 *
 *   Line  1: '#' bar graph  (1 hash per 100mV of avg_mv)
 *   Line  2: scale line     |----|----| ... |
 *   Line  3: voltage labels 0    0.5  ... 3.0
 *   Line  4: (blank)
 *   Line  5: ┌─────┬────────┬──────────────┐
 *   Line  6: │ ADC │ counts │    volts     │
 *   Line  7: ├─────┼────────┼──────────────┤
 *   Line  8: │ MIN │  XXXX  │   X.XXX V    │
 *   Line  9: ├─────┼────────┼──────────────┤
 *   Line 10: │ MAX │  XXXX  │   X.XXX V    │
 *   Line 11: ├─────┼────────┼──────────────┤
 *   Line 12: │ AVG │  XXXX  │   X.XXX V    │
 *   Line 13: └─────┴────────┴──────────────┘
 */
void LPUART_print_ADC_table(uint16_t min_counts,
                            uint16_t max_counts,
                            uint16_t avg_counts,
                            int32_t  min_mv,
                            int32_t  max_mv,
                            int32_t  avg_mv)
{
    LPUART_cursor_home();

    /* Bar graph: hashes, scale, labels */
    LPUART_print_bar_graph(avg_mv);

    /* Blank separator line */
    LPUART_print("\r\n");

    /* Top border: ┌─────┬────────┬──────────────┐ */
    LPUART_print_hline(TL, TT, TR);

    /* Header: │ ADC │ counts │    volts     │ */
    LPUART_print_spaces(CENTER_PAD);
    LPUART_print_char(VL);
    LPUART_print(" ADC ");           /*  5 chars col 1 */
    LPUART_print_char(VL);
    LPUART_print(" counts ");        /*  8 chars col 2 */
    LPUART_print_char(VL);
    LPUART_print("    volts     ");  /* 14 chars col 3 */
    LPUART_print_char(VL);
    LPUART_print("\r\n");

    /* Mid border + MIN row */
    LPUART_print_hline(LT, CR, RT);
    LPUART_print_table_row("MIN", min_counts, min_mv);

    /* Mid border + MAX row */
    LPUART_print_hline(LT, CR, RT);
    LPUART_print_table_row("MAX", max_counts, max_mv);

    /* Mid border + AVG row */
    LPUART_print_hline(LT, CR, RT);
    LPUART_print_table_row("AVG", avg_counts, avg_mv);

    /* Bottom border: └─────┴────────┴──────────────┘ */
    LPUART_print_hline(BL, BT, BR);
}
