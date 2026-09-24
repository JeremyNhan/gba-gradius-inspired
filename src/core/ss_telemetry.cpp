#include "ss_telemetry.h"

// Constant-initialised (all fields have default member initialisers), so it lives in .data and the
// magic string is valid from the very first instruction.
extern "C"
{
    volatile ss_telemetry_block ss_telemetry;
}
