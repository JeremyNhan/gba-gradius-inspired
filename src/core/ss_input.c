#include "ss_input.h"

static u16 held;
static u16 previous;

#if SS_TESTS
static int injected = -1;

void input_inject(int keys)
{
    injected = keys;
}
#endif

void input_update(void)
{
    previous = held;
    held = (u16) (~REG_KEYINPUT & KEY_MASK);

#if SS_TESTS
    if(injected >= 0)
    {
        held = (u16) injected;
    }
#endif
}

bool input_held(u16 key)
{
    return (held & key) != 0;
}

bool input_pressed(u16 key)
{
    return (held & key) && ! (previous & key);
}
