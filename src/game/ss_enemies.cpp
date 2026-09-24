#include "ss_enemies.h"

#include "bn_sprite_items_asteroid_big.h"
#include "bn_sprite_items_asteroid_small.h"
#include "bn_sprite_items_enemy_dart.h"
#include "bn_sprite_items_enemy_hulk.h"
#include "bn_sprite_items_enemy_interceptor.h"
#include "bn_sprite_items_enemy_mine.h"
#include "bn_sprite_items_enemy_swarm.h"
#include "bn_sprite_items_enemy_turret.h"
#include "bn_sprite_items_enemy_waver.h"
#include "bn_sprite_palette_items_flash_master.h"

#include "ss_audio.h"
#include "ss_math.h"
#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    //  item                                    frm anim hw hh  hp  score  move                  speed           fire                      intv first from explosion             immune drop%
    constexpr enemy_def enemy_defs[] = {
        { &bn::sprite_items::enemy_dart,         2, 4,  6, 4,  2,  100, move_type::STRAIGHT,  bn::fixed(1.8), fire_type::AIMED,          220,  70, 1, explosion_size::SMALL, false,   8 },
        { &bn::sprite_items::enemy_waver,        3, 6,  6, 4,  2,  150, move_type::SINE,      bn::fixed(1.2), fire_type::STRAIGHT,       200, 110, 0, explosion_size::SMALL, false,   8 },
        { &bn::sprite_items::enemy_interceptor,  2, 3,  6, 3,  3,  250, move_type::INTERCEPT, bn::fixed(4.0), fire_type::BURST3,         999,  20, 0, explosion_size::SMALL, false,  12 },
        { &bn::sprite_items::enemy_turret,       5, 0,  6, 5,  5,  300, move_type::GROUND,    bn::fixed(0),   fire_type::AIMED,          110,  40, 0, explosion_size::SMALL, true,  15 },
        { &bn::sprite_items::enemy_hulk,         2, 8, 13, 10, 40, 2000, move_type::HOVER,    bn::fixed(0.8), fire_type::AIMED_SPREAD3,   80,  50, 0, explosion_size::BIG,   true, 100 },
        { &bn::sprite_items::enemy_swarm,        4, 4,  5, 5,  1,  100, move_type::LOOP,      bn::fixed(2.0), fire_type::NONE,             0,   0, 0, explosion_size::SMALL, false,   5 },
        { &bn::sprite_items::enemy_mine,         2, 10, 5, 5,  4,  200, move_type::DRIFT,     bn::fixed(0.6), fire_type::RING8,            0,   0, 0, explosion_size::SMALL, false,  10 },
        { &bn::sprite_items::asteroid_small,     4, 12, 6, 6,  5,   50, move_type::TUMBLE,    bn::fixed(1.0), fire_type::NONE,             0,   0, 0, explosion_size::SMALL, true,   3 },
        { &bn::sprite_items::asteroid_big,       4, 16, 12, 12, 18, 400, move_type::TUMBLE,   bn::fixed(0.6), fire_type::NONE,             0,   0, 0, explosion_size::BIG,   true,  25 },
    };

    static_assert(sizeof(enemy_defs) / sizeof(enemy_defs[0]) == int(enemy_kind::COUNT));

    constexpr int spawn_x = 136;
}

hitbox enemy::box() const
{
    return make_hitbox(position, def->half_w, def->half_h);
}

int enemies::_alloc_formation(bool carrier)
{
    for(int index = 0; index < max_formations; ++index)
    {
        formation_info& info = _formations[index];

        if(! info.used)
        {
            info = formation_info();
            info.used = true;
            info.carrier = carrier;
            return index;
        }
    }

    return -1;
}

void enemies::spawn(world& w, enemy_kind kind, bn::fixed x, bn::fixed y, int param, short flags, int formation)
{
    enemy* e = _pool.spawn();

    if(! e)
    {
        return;
    }

    const enemy_def& def = enemy_defs[int(kind)];
    e->kind = kind;
    e->def = &def;
    e->position = bn::fixed_point(x, y);
    e->base_y = y;
    e->hp = def.hp;
    e->timer = 0;
    e->param = param;
    e->phase = 0;
    e->angle = 0;
    e->burst = 0;
    e->flash = 0;
    e->flags = flags;
    e->entered = false;
    e->formation = (signed char) formation;
    e->velocity = bn::fixed_point(-def.speed, 0);
    e->fire_timer = def.first_fire_delay + (w.rng.get_int(30));

    if(def.move == move_type::STRAIGHT || def.move == move_type::TUMBLE)
    {
        e->velocity.set_y(bn::fixed(param) / 100);
    }

    if(flags & flag_from_left)
    {
        e->velocity.set_x(-e->velocity.x());
    }

    if(formation >= 0)
    {
        ++_formations[formation].alive;
        ++_formations[formation].spawned;
    }

    e->frame = 0;
    e->sprite = make_sprite(*def.item, e->position, 0, z_enemies, w.camera);

    if(e->sprite)
    {
        if(flags & flag_from_left)
        {
            e->sprite->set_horizontal_flip(true);
        }

        if(flags & flag_ceiling)
        {
            e->sprite->set_vertical_flip(true);
        }
    }
}

void enemies::spawn_formation(world& w, formation_type type, int y, int count, short flags)
{
    int formation = _alloc_formation(flags & flag_carrier);
    short member_flags = short(flags & ~flag_carrier);

    for(int index = 0; index < count; ++index)
    {
        int mid = count / 2;

        switch(type)
        {

        case formation_type::LINE:
            spawn(w, enemy_kind::DART, spawn_x + index * 20, y, 0, member_flags, formation);
            break;

        case formation_type::V:
            spawn(w, enemy_kind::DART, spawn_x + bn::abs(index - mid) * 16, y + (index - mid) * 14, 0, member_flags,
                  formation);
            break;

        case formation_type::COLUMN:
            spawn(w, enemy_kind::DART, spawn_x, y + (index - mid) * 18, 0, member_flags, formation);
            break;

        case formation_type::WAVE:
            spawn(w, enemy_kind::WAVER, spawn_x + index * 22, y, 28, member_flags, formation);
            break;

        case formation_type::SWARM_LOOP:
            spawn(w, enemy_kind::SWARM, spawn_x + index * 16, y, 30, member_flags, formation);
            break;

        case formation_type::MINE_FIELD:
            spawn(w, enemy_kind::MINE, spawn_x + index * 34, y + ((index * 37) % 90) - 45, 0, member_flags,
                  formation);
            break;

        default:
            break;
        }
    }
}

void enemies::_formation_member_gone(world& w, enemy& target, bool killed)
{
    if(target.formation < 0)
    {
        return;
    }

    formation_info& info = _formations[int(target.formation)];
    --info.alive;

    if(killed)
    {
        ++info.killed;
    }

    if(info.alive <= 0)
    {
        // Bonus for wiping out a whole bonus formation.
        if(info.carrier && info.killed == info.spawned)
        {
            w.add_score(500);
        }

        info.used = false;
    }
}

void enemies::_leave(world& w, enemy& target)
{
    _formation_member_gone(w, target, false);
    _pool.release(target);
}

void enemies::damage(world& w, enemy& target, int amount)
{
    target.hp -= amount;

    if(target.hp <= 0)
    {
        destroy(w, target, true);
        return;
    }

    target.flash = 3;

    if(target.sprite)
    {
        target.sprite->set_palette(bn::sprite_palette_items::flash_master);
    }

    audio::play(audio::sfx::HIT);
}

void enemies::destroy(world& w, enemy& target, bool by_player)
{
    const enemy_def& def = *target.def;
    bn::fixed_point position = target.position;

    if(def.explosion == explosion_size::BIG)
    {
        w.fx.explosion_big(w, position);
        w.shake(12, 2);
        audio::play(audio::sfx::EXPLODE_BIG);
    }
    else
    {
        w.fx.explosion_small(w, position);
        audio::play(audio::sfx::EXPLODE);
    }

    if(by_player)
    {
        w.add_score(def.score);

        // Power capsules are a random reward for kills (deterministic: the world RNG is seeded).
        if(def.drop_chance && w.rng.get_int(100) < def.drop_chance)
        {
            w.items.drop(w, position);
        }

        if(target.kind == enemy_kind::MINE)
        {
            w.bullets.fire_ring(w, bullet_kind::SMALL, position, 8, bn::fixed(1.3), w.stage_frame * 512);
        }
    }

    bool split = target.kind == enemy_kind::ASTEROID_BIG && by_player;
    _formation_member_gone(w, target, by_player);
    _pool.release(target);

    if(split)
    {
        spawn(w, enemy_kind::ASTEROID_SMALL, position.x(), position.y() - 6, -70, 0);
        spawn(w, enemy_kind::ASTEROID_SMALL, position.x() + 4, position.y(), 0, 0);
        spawn(w, enemy_kind::ASTEROID_SMALL, position.x(), position.y() + 6, 70, 0);
    }
}

void enemies::destroy_all(world& w)
{
    int delay = 0;

    for(enemy& e : _pool)
    {
        if(e.active)
        {
            // Stagger the explosions over several frames to spread the sprite creation cost.
            w.fx.explosion_small(w, e.position, 1 + delay);
            delay += 3;
            _formation_member_gone(w, e, false);
            _pool.release(e);
        }
    }
}

void enemies::shockwave(world& w)
{
    int delay = 0;

    for(enemy& e : _pool)
    {
        if(e.active)
        {
            // Stagger the explosions over several frames to spread the sprite creation cost.
            w.add_score(e.def->score);
            w.fx.explosion_small(w, e.position, 1 + delay);
            delay += 2;
            _formation_member_gone(w, e, false);
            _pool.release(e);
        }
    }
}

bn::optional<bn::fixed_point> enemies::nearest_target(const bn::fixed_point& from) const
{
    bn::optional<bn::fixed_point> result;
    int best = 0x7FFFFFFF;

    for(const enemy& e : _pool)
    {
        if(e.active && e.entered && e.position.x() > from.x() - 16)
        {
            int dx = (e.position.x() - from.x()).integer();
            int dy = (e.position.y() - from.y()).integer();
            int distance = dx * dx + dy * dy;

            if(distance < best)
            {
                best = distance;
                result = e.position;
            }
        }
    }

    return result;
}

void enemies::_move(world& w, enemy& e)
{
    const enemy_def& def = *e.def;
    const bn::fixed_point& target = w.ship.position();

    switch(def.move)
    {

    case move_type::STRAIGHT:
    case move_type::TUMBLE:
        e.position += e.velocity;
        break;

    case move_type::SINE:
        {
            int amplitude = e.param ? e.param : 24;
            e.position.set_x(e.position.x() - def.speed);
            bn::fixed_point wave = direction(e.timer * 512 + e.angle, amplitude);
            e.position.set_y(e.base_y + wave.y());
        }
        break;

    case move_type::INTERCEPT:
        if(e.phase == 0)
        {
            bn::fixed stop_x = e.param ? e.param : 70;
            bn::fixed step = (e.position.x() - stop_x) / 12;

            if(step < bn::fixed(0.4))
            {
                e.phase = 1;
                e.timer = 0;
            }
            else
            {
                e.position.set_x(e.position.x() - step);
            }
        }
        else if(e.phase == 1)
        {
            if(e.timer == 10 && w.ship.alive())
            {
                e.burst = 3;
                e.fire_timer = 0;
            }

            if(e.timer >= 45)
            {
                e.phase = 2;
                int dash_angle = w.ship.alive() ? angle_to(e.position, target) : angle_left;

                // never dash backwards: clamp to the left half-plane
                if(bn::abs(angle_delta(angle_left, dash_angle)) > 12000)
                {
                    dash_angle = angle_left + (angle_delta(angle_left, dash_angle) > 0 ? 12000 : -12000);
                }

                e.velocity = direction(dash_angle, def.speed);
            }
        }
        else
        {
            e.position += e.velocity;
        }
        break;

    case move_type::GROUND:
        e.position.set_x(e.position.x() - w.scroll_speed);
        break;

    case move_type::HOVER:
        if(e.phase == 0)
        {
            e.position.set_x(e.position.x() - def.speed);

            if(e.position.x() <= 70)
            {
                e.phase = 1;
                e.timer = 0;
            }
        }
        else if(e.phase == 1)
        {
            e.position.set_y(e.base_y + direction(e.timer * 300, 28).y());

            if(e.timer > 480)
            {
                e.phase = 2;
            }
        }
        else
        {
            e.position.set_x(e.position.x() - def.speed * 2);
        }
        break;

    case move_type::LOOP:
        if(e.phase == 0)
        {
            e.position.set_x(e.position.x() - def.speed);

            if(e.position.x() <= e.param)
            {
                e.phase = 1;
                e.timer = 0;
                e.angle = angle_left;
            }
        }
        else if(e.phase == 1)
        {
            // Rotate the heading through a full circle in 64 frames (upwards loop in the bottom
            // half of the screen, downwards in the top half).
            e.angle = (e.angle + (e.base_y > 0 ? 1024 : -1024)) & 0xFFFF;
            e.position += direction(e.angle, def.speed);

            if(e.timer >= 64)
            {
                e.phase = 2;
            }
        }
        else
        {
            e.position.set_x(e.position.x() - def.speed * bn::fixed(1.3));
        }
        break;

    case move_type::DRIFT:
        {
            bn::fixed vy = e.velocity.y();

            if(w.ship.alive())
            {
                vy += target.y() > e.position.y() ? bn::fixed(0.01) : bn::fixed(-0.01);
            }

            if(vy > bn::fixed(0.45))
            {
                vy = bn::fixed(0.45);
            }
            else if(vy < bn::fixed(-0.45))
            {
                vy = bn::fixed(-0.45);
            }

            e.velocity.set_y(vy);
            e.position += e.velocity;
        }
        break;

    default:
        break;
    }
}

void enemies::_fire(world& w, enemy& e)
{
    const enemy_def& def = *e.def;

    if(! w.ship.alive() || ! e.entered || e.position.x() < -100 || e.position.x() > 112)
    {
        return;
    }

    // Burst in progress (interceptors, BURST3).
    if(e.burst > 0)
    {
        if(--e.fire_timer <= 0)
        {
            w.bullets.fire_aimed(w, bullet_kind::SMALL, e.position, bn::fixed(2.2) + bn::fixed(0.2) * w.difficulty(), 0);
            --e.burst;
            e.fire_timer = 6;
        }

        return;
    }

    if(def.fire == fire_type::NONE || def.fire == fire_type::RING8 || def.move == move_type::INTERCEPT ||
       w.difficulty() < def.fire_from_stage)
    {
        return;
    }

    if(--e.fire_timer > 0)
    {
        return;
    }

    // Harder stages shorten the interval: x1, x0.67, x0.5.
    e.fire_timer = (def.fire_interval * 4) / (4 + w.difficulty() * 2);

    // Don't shoot point-blank.
    bn::fixed dx = w.ship.position().x() - e.position.x();

    if(bn::abs(dx) < 24 && bn::abs(w.ship.position().y() - e.position.y()) < 24)
    {
        return;
    }

    bn::fixed speed = bn::fixed(1.5) + bn::fixed(0.2) * w.difficulty();
    bn::fixed_point muzzle = e.position;

    if(def.move == move_type::GROUND)
    {
        muzzle += direction(angle_to(e.position, w.ship.position()), 7);
    }

    switch(def.fire)
    {

    case fire_type::AIMED:
        w.bullets.fire_aimed(w, bullet_kind::SMALL, muzzle, speed, 0);
        break;

    case fire_type::AIMED_SPREAD3:
        w.bullets.fire_aimed(w, bullet_kind::BIG, muzzle, speed, 0);
        w.bullets.fire_aimed(w, bullet_kind::BIG, muzzle, speed, degrees(18));
        w.bullets.fire_aimed(w, bullet_kind::BIG, muzzle, speed, degrees(-18));
        break;

    case fire_type::BURST3:
        e.burst = 3;
        e.fire_timer = 0;
        break;

    case fire_type::STRAIGHT:
        w.bullets.fire(w, bullet_kind::NEEDLE, muzzle, bn::fixed_point(-speed - 1, 0));
        break;

    default:
        break;
    }
}

void enemies::_animate(world& w, enemy& e)
{
    const enemy_def& def = *e.def;
    int frame = e.frame;

    if(def.move == move_type::GROUND)
    {
        // Barrel frames: 0 = left, 2 = up, 4 = right (visual angles 180..0 degrees).
        int visual = (angle_turn - angle_to(e.position, w.ship.position())) & 0xFFFF;

        if(e.flags & flag_ceiling)
        {
            visual = (angle_turn - visual) & 0xFFFF;
        }

        if(visual > 32768)
        {
            frame = visual > 49152 ? 4 : 0;
        }
        else
        {
            frame = 4 - ((visual + 4096) >> 13);
            frame = bn::clamp(frame, 0, 4);
        }
    }
    else if(def.anim_period)
    {
        frame = (e.timer / def.anim_period) % def.frames;
    }

    set_frame(e.sprite, *def.item, e.frame, frame);

    if(e.flash && --e.flash == 0 && e.sprite)
    {
        e.sprite->set_palette(def.item->palette_item());
    }
}

void enemies::update(world& w)
{
    for(enemy& e : _pool)
    {
        if(! e.active)
        {
            continue;
        }

        ++e.timer;
        _move(w, e);

        bn::fixed x = e.position.x();
        bn::fixed y = e.position.y();

        if(! e.entered && x < 112 && x > -112 && y > -76 && y < 76)
        {
            e.entered = true;
        }

        bool gone = x < -150 || x > 260 || y < -130 || y > 130 || (e.entered && off_screen(e.position, 40));

        if(gone)
        {
            _leave(w, e);
            continue;
        }

        _fire(w, e);
        _animate(w, e);

        if(e.sprite)
        {
            e.sprite->set_position(e.position);
        }
    }
}

}
