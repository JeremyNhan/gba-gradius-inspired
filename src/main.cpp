/*
 * Space Shooter — a horizontal shoot-'em-up for the Game Boy Advance.
 * Built with Butano (https://github.com/GValiente/butano) on devkitARM.
 */

#include <new>

#include "bn_core.h"
#include "bn_timer.h"
#include "bn_timers.h"

#include "ss_app.h"
#include "ss_constants.h"
#include "ss_telemetry.h"

namespace
{
    // The whole game (screens, world, entity pools) lives in EWRAM: IWRAM is only 32 KB and is
    // reserved for the stack and Butano's hot data. No heap allocation is used.
    BN_DATA_EWRAM_BSS alignas(ss::app) unsigned char app_storage[sizeof(ss::app)];
}

int main()
{
    bn::core::init();

    ss::app* game = ::new(static_cast<void*>(app_storage)) ss::app();

    while(true)
    {
#if SS_TEST_HOOKS
        // Game logic cost (everything but Butano's own frame work), in 1/1000 frame.
        bn::timer logic_timer;
        game->update();
        ss_telemetry.prof_app = uint16_t(logic_timer.elapsed_ticks() * 1000 / bn::timers::ticks_per_frame());
#else
        game->update();
#endif
        bn::core::update();
    }
}
