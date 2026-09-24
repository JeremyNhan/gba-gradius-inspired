/*
 * Keypad state, read once per frame. Test builds can replace the hardware keys with scripted ones
 * (input_inject), so the automated scenarios drive the real game code.
 */
#ifndef SS_INPUT_H
#define SS_INPUT_H

#include "ss_base.h"

void input_update(void);

bool input_held(u16 key);       /* KEY_A, KEY_B, KEY_START, ... (tonc KEY_* masks) */
bool input_pressed(u16 key);    /* went down this frame */

#if SS_TESTS
/* Keys used instead of the keypad from the next input_update() on; negative = hardware again. */
void input_inject(int keys);
#endif

#endif
