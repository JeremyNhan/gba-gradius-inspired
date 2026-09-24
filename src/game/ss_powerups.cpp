#include "ss_powerups.h"

#include "bn_sprite_items_powerup.h"

#include "ss_audio.h"
#include "ss_math.h"
#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    // Deterministic drop order. Upgrades that are already maxed are skipped when dropping.
    constexpr powerup_type rotation[] = {
        powerup_type::SHOT, powerup_type::MISSILE, powerup_type::SPEED, powerup_type::SPREAD,
        powerup_type::SHIELD, powerup_type::SHOT, powerup_type::MISSILE, powerup_type::SPEED,
        powerup_type::SHIELD, powerup_type::SPREAD, powerup_type::LIFE, powerup_type::SHOT
    };

    constexpr int rotation_size = sizeof(rotation) / sizeof(rotation[0]);
}

bool powerups::_useful(const world& w, powerup_type type) const
{
    const loadout& gear = w.game.gear;

    switch(type)
    {

    case powerup_type::SPEED:
        return gear.speed_level < max_speed_level;

    case powerup_type::SHOT:
        return gear.weapon != weapon_type::NORMAL || gear.weapon_level < max_weapon_level;

    case powerup_type::SPREAD:
        return gear.weapon != weapon_type::SPREAD || gear.weapon_level < max_weapon_level;

    case powerup_type::MISSILE:
        return gear.missile_level < max_missile_level;

    case powerup_type::SHIELD:
        return gear.shield < max_shield;

    case powerup_type::LIFE:
        return w.game.lives < max_lives;

    default:
        return false;
    }
}

void powerups::drop(world& w, const bn::fixed_point& position)
{
    powerup_type type = rotation[_rotation % rotation_size];

    for(int attempt = 0; attempt < rotation_size; ++attempt)
    {
        type = rotation[_rotation % rotation_size];
        ++_rotation;

        if(_useful(w, type))
        {
            break;
        }
    }

    powerup* item = _pool.spawn();

    if(! item)
    {
        return;
    }

    item->type = type;
    item->position = position;
    item->timer = 0;
    item->sprite = make_sprite(bn::sprite_items::powerup, position, int(type), z_powerups, w.camera);
}

void powerups::collect(world& w, powerup& item)
{
    loadout& gear = w.game.gear;
    bool upgraded = true;

    switch(item.type)
    {

    case powerup_type::SPEED:
        upgraded = gear.speed_level < max_speed_level;
        gear.speed_level = bn::min(gear.speed_level + 1, max_speed_level);
        break;

    case powerup_type::SHOT:
    case powerup_type::SPREAD:
        {
            weapon_type wanted = item.type == powerup_type::SHOT ? weapon_type::NORMAL : weapon_type::SPREAD;

            if(gear.weapon != wanted)
            {
                gear.weapon = wanted;
            }
            else
            {
                upgraded = gear.weapon_level < max_weapon_level;
                gear.weapon_level = bn::min(gear.weapon_level + 1, max_weapon_level);
            }
        }
        break;

    case powerup_type::MISSILE:
        upgraded = gear.missile_level < max_missile_level;
        gear.missile_level = bn::min(gear.missile_level + 1, max_missile_level);
        break;

    case powerup_type::SHIELD:
        gear.shield = max_shield;
        w.ship.refresh_shield_sprite(w);
        break;

    case powerup_type::LIFE:
        upgraded = w.game.lives < max_lives;
        w.game.lives = bn::min(w.game.lives + 1, max_lives);
        break;

    default:
        break;
    }

    w.add_score(upgraded ? 200 : 1000);
    w.display.show_pickup(item.type);
    audio::play(item.type == powerup_type::LIFE ? audio::sfx::ONE_UP : audio::sfx::PICKUP);
    _pool.release(item);
}

void powerups::update(world& w)
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
        }
    }
}

}
