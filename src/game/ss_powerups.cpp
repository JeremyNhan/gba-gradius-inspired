#include "ss_powerups.h"

#include "bn_sprite_items_powerup.h"

#include "ss_audio.h"
#include "ss_math.h"
#include "ss_power_data.h"
#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

void powerups::drop(world& w, const bn::fixed_point& position)
{
    powerup* item = _pool.spawn();

    if(! item)
    {
        return;     // four capsules already on screen: this drop is lost
    }

    item->position = position;
    item->timer = 0;
    item->frame = 0;
    item->sprite = make_sprite(bn::sprite_items::powerup, position, 0, z_powerups, w.camera);
}

void powerups::collect(world& w, powerup& item)
{
    int power = w.game.gear.power;

    if(power < max_power)
    {
        w.set_power(power + 1);
        w.add_score(200);
        w.display.show_pickup(power_names[power + 1]);
    }
    else
    {
        // Full power: refill the shield and score a bonus.
        w.game.gear.shield = max_shield;
        w.ship.refresh_power(w, power);
        w.add_score(1000);
        w.display.show_pickup("FULL POWER");
    }

    audio::play(w.game.gear.power == max_power && power < max_power ? audio::sfx::POWER_MAX : audio::sfx::PICKUP);
    _pool.release(item);
}

void powerups::update(world&)
{
    for(powerup& item : _pool)
    {
        if(! item.active)
        {
            continue;
        }

        ++item.timer;

        // Drift left slowly while bobbing; blink during the last two seconds before expiring.
        bn::fixed bob = direction(item.timer * 1024, bn::fixed(0.35)).y();
        item.position += bn::fixed_point(bn::fixed(-0.45), bob);

        if(item.position.y() < play_top + 8)
        {
            item.position.set_y(play_top + 8);
        }
        else if(item.position.y() > play_bottom - 8)
        {
            item.position.set_y(play_bottom - 8);
        }

        if(item.position.x() < -130 || item.timer > 900)
        {
            _pool.release(item);
            continue;
        }

        if(item.sprite)
        {
            item.sprite->set_position(item.position);
            item.sprite->set_visible(item.timer < 780 || (item.timer & 4));
            set_frame(item.sprite, bn::sprite_items::powerup, item.frame, (item.timer >> 3) & 1);
        }
    }
}

}
