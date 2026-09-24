/*
 * Text on BG0 with the original 5x7 font at a 6 px pitch (fits a 40-column HUD line on 240 px).
 *
 * Every screen row (20 rows of 8 px) owns 30 BG tiles; strings are drawn into those tiles in RAM
 * and the changed rows are copied to VRAM during VBlank. Colours are BG palette banks per 8 px cell,
 * so two strings on one row must not share a cell.
 */
#ifndef SS_TEXT_H
#define SS_TEXT_H

#include "ss_base.h"

#define TEXT_ROWS 20
#define TEXT_PITCH 6

typedef enum
{
    TEXT_WHITE,
    TEXT_YELLOW,
    TEXT_CYAN,
    TEXT_RED
} text_color;

void text_init(void);
void text_clear_all(void);
void text_clear_row(int row);

/* Draws a string whose first character starts at pixel x (0..239) of a row. */
void text_print(int row, int x, const char* s, text_color color);

/* Horizontally centred on the screen. */
void text_center(int row, const char* s, text_color color);

int text_width(const char* s);

/* Copies changed rows to VRAM (call in VBlank). */
void text_commit(void);

/* ----- small string helpers (no printf on the GBA side) ------------------------------------------ */

/* Appends a number with leading zeros to at least `digits` digits. Returns the new end of `out`. */
char* text_append_number(char* out, int value, int digits);
char* text_append(char* out, const char* s);

#endif
