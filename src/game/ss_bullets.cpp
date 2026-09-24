#include "ss_bullets.h"

#include "bn_sprite_items_bullet_big.h"
#include "bn_sprite_items_bullet_needle.h"
#include "bn_sprite_items_bullet_small.h"

#include "ss_math.h"
#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    const bn::sprite_item& item_of(bullet_kind kind)
    {
        switch(kind)
        {

        case bullet_kind::BIG:
            return bn::sprite_items::bullet_big;

        case bullet_kind::NEEDLE:
            return bn::sprite_items::bullet_needle;

        default:
            return bn::sprite_items::bullet_small;
        }
    }
}

void enemy_bullets::fire(world& w, bullet_kind kind, const bn::fixed_point& position, const bn::fixed_point& velocity)
{
    enemy_bullet* bullet = _pool.spawn();

    if(! bullet)
    {
        return;
    }

    bullet->kind = kind;
    bullet->position = position;
    bullet->velocity = velocity;
    bullet->timer = 0;
    bullet->frame = 0;
    bullet->sprite = make_sprite(item_of(kind), position, 0, z_bullets, w.camera);

    if(bullet->sprite && kind == bullet_kind::NEEDLE && velocity.x() > 0)
    {
        bullet->sprite->set_horizontal_flip(true);
    }
}

void enemy_bullets::fire_aimed(world& w, bullet_kind kind, const bn::fixed_point& position, bn::fixed speed,
                               int angle_offset)
{
    int angle = angle_to(position, w.ship.position()) + angle_offset;
    fire(w, kind, position, direction(angle, speed));
}

void enemy_bullets::fire_ring(world& w, bullet_kind kind, const bn::fixed_point& position, int count, bn::fixed speed,
                              int phase)
{
    int step = angle_turn / count;

    for(int index = 0; index < count; ++index)
    {
        fire(w, kind, position, direction(phase + index * step, speed));
    }
}

void enemy_bullets::fire_fan(world& w, bullet_kind kind, const bn::fixed_point& position, int count, bn::fixed speed,
                             int spread_step)
{
    int first = -(count - 1) * spread_step / 2;

    for(int index = 0; index < count; ++index)
    {
        fire_aimed(w, kind, position, speed, first + index * spread_step);
    }
}

void enemy_bullets::update(world& w)
{
    for(enemy_bullet& bullet : _pool)
    {
        if(! bullet.active)
        {
            continue;
        }

        bullet.position += bullet.velocity;
        ++bullet.timer;

        if(off_screen(bullet.position, 8) || w.ground.blocks(bullet.box(), w.scroll_x))
        {
            _pool.release(bullet);
            continue;
        }

        if(bullet.kind != bullet_kind::NEEDLE)
        {
            set_frame(bullet.sprite, item_of(bullet.kind), bullet.frame, (bullet.timer >> 3) & 1);
        }

        if(bullet.sprite)
        {
            bullet.sprite->set_position(bullet.position);
        }
    }
}

void enemy_bullets::cancel_all(world& w)
{
    // Only a handful of bullets get a spark: creating dozens of sprites in a single frame would
    // blow the frame budget (measured: >100% CPU on boss phase changes without this cap).
    constexpr int max_sparks = 5;
    int sparks = 0;

    for(enemy_bullet& bullet : _pool)
    {
        if(bullet.active)
        {
            if(sparks < max_sparks && (_pool.index_of(bullet) & 3) == 0)
            {
                w.fx.spark(w, bullet.position);
                ++sparks;
            }

            _pool.release(bullet);
        }
    }
}

}
