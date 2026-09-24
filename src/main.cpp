/*
 * Space Shooter — a horizontal shoot-'em-up for the Game Boy Advance.
 * Built with Butano (https://github.com/GValiente/butano) on devkitARM.
 */

#include <new>

#include "bn_core.h"

#include "ss_app.h"

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
        game->update();
        bn::core::update();
    }
}
