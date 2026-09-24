#include "ss_telemetry.h"

// Everything but the magic starts at zero; the game fills it in every frame.
extern "C" volatile ss_telemetry_block ss_telemetry = { { 'S', 'S', 'T', 'E', 'L', 'E', 'M', '1' } };
