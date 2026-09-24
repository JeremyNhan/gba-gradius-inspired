#include "ss_boss.h"

#include "bn_sprite_items_boss_hive.h"
#include "bn_sprite_items_boss_overmind_front.h"
#include "bn_sprite_items_boss_overmind_rear.h"
#include "bn_sprite_items_boss_pod.h"
#include "bn_sprite_items_boss_warden.h"
#include "bn_sprite_palette_items_flash_boss.h"

#include "ss_audio.h"
#include "ss_math.h"
#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    struct boss_info
    {
        const bn::sprite_item* body;
        int hp;
        int score;
        int home_x;
        int dying_frames;
    };

    constexpr boss_info boss_infos[] = {
        { &bn::sprite_items::boss_warden, 300, 10000, 64, 150 },
        { &bn::sprite_items::boss_hive, 420, 20000, 60, 150 },
        { &bn::sprite_items::boss_overmind_front, 640, 50000, 24, 240 },
    };

    constexpr int pod_hp = 30;
    constexpr int overmind_rear_offset = 64;
}

void boss::start(world& w, boss_id id)
{
    const boss_info& info = boss_infos[int(id)];
    _id = id;
    _state = state::ENTER;
    _hp = info.hp;
    _hp_max = info.hp;
    _phase = 0;
    _timer = 0;
    _attack_timer = 60;
    _flash = 0;
    _open_frames = 0;
    _cycle = 0;
    _home_x = info.home_x;
    _position = bn::fixed_point(180, 0);
    _velocity = bn::fixed_point();
    _body_frame = 0;
    _body = make_sprite(*info.body, _position, 0, z_boss, w.camera);

    if(id == boss_id::OVERMIND)
    {
        _rear = make_sprite(bn::sprite_items::boss_overmind_rear, _position, 0, z_boss + 1, w.camera);
        _rear_frame = 0;

        for(pod& p : _pods)
        {
            p.alive = true;
            p.hp = pod_hp;
            p.flash = 0;
            p.frame = 0;
            p.sprite = make_sprite(bn::sprite_items::boss_pod, _position, 0, z_boss - 1, w.camera);
        }
    }

    _sync_sprites(w);
}

hitbox boss::_core_box() const
{
    switch(_id)
    {

    case boss_id::WARDEN:
        return make_hitbox(_position + bn::fixed_point(4, 0), 27, 20);

    case boss_id::HIVE:
        return make_hitbox(_position, 15, 15);

    default:
        return make_hitbox(_position + bn::fixed_point(4, 0), 24, 22);
    }
}

bn::fixed_point boss::_muzzle() const
{
    switch(_id)
    {

    case boss_id::WARDEN:
        return _position + bn::fixed_point(-18, 0);

    case boss_id::HIVE:
        return _position;

    default:
        return _position + bn::fixed_point(-8, 0);
    }
}

bn::optional<bn::fixed_point> boss::target_point() const
{
    if(_state == state::FIGHT)
    {
        return _position;
    }

    return bn::optional<bn::fixed_point>();
}

bool boss::touches(const hitbox& box) const
{
    if(_state != state::FIGHT && _state != state::ENTER)
    {
        return false;
    }

    if(_core_box().intersects(box))
    {
        return true;
    }

    if(_id == boss_id::OVERMIND)
    {
        if(make_hitbox(_position + bn::fixed_point(overmind_rear_offset, 0), 28, 26).intersects(box))
        {
            return true;
        }

        for(const pod& p : _pods)
        {
            if(p.alive && make_hitbox(p.position, 6, 5).intersects(box))
            {
                return true;
            }
        }
    }

    return false;
}

void boss::_set_flash(bool flash)
{
    const bn::sprite_palette_item& palette = flash ? bn::sprite_palette_items::flash_boss :
                                                     boss_infos[int(_id)].body->palette_item();

    if(_body)
    {
        _body->set_palette(palette);
    }

    if(_rear)
    {
        _rear->set_palette(palette);
    }
}

bool boss::take_hit(world& w, const hitbox& box, int damage)
{
    if(_state != state::FIGHT)
    {
        return false;
    }

    // Pods first: they sit in front of the final boss.
    if(_id == boss_id::OVERMIND)
    {
        for(pod& p : _pods)
        {
            if(p.alive && make_hitbox(p.position, 7, 6).intersects(box))
            {
                p.hp -= damage;
                p.flash = 3;

                if(p.hp <= 0)
                {
                    p.alive = false;
                    p.sprite.reset();
                    w.fx.explosion_big(w, p.position);
                    w.add_score(3000);
                    audio::play(audio::sfx::EXPLODE_BIG);
                }
                else if(p.sprite)
                {
                    p.sprite->set_palette(bn::sprite_palette_items::flash_boss);
                    audio::play(audio::sfx::HIT);
                }

                return true;
            }
        }
    }

    if(! _core_box().intersects(box))
    {
        return false;
    }

    // The final boss is armoured while its eye is closed (phase 0): half damage.
    if(_id == boss_id::OVERMIND && _phase == 0)
    {
        damage = (damage + 1) / 2;
    }

    _hp -= damage;
    _flash = 2;
    _set_flash(true);
    audio::play(audio::sfx::HIT);

    int new_phase = _hp * 3 > _hp_max * 2 ? 0 : _hp * 3 > _hp_max ? 1 : 2;

    if(new_phase != _phase && _hp > 0)
    {
        _phase = new_phase;
        _attack_timer = 50;
        _cycle = 0;
        w.shake(20, 2);
        w.bullets.cancel_all(w);
        audio::play(audio::sfx::EXPLODE_BIG);
        w.fx.explosion_big(w, _position + bn::fixed_point(w.rng.get_int(-16, 16), w.rng.get_int(-16, 16)));
    }

    if(_hp <= 0)
    {
        // Heavy cleanup (bullet cancel, enemy explosions) runs over the next frames in
        // _update_dying() so that this frame stays within budget.
        _hp = 0;
        _state = state::DYING;
        _timer = 0;
        audio::stop_music();
    }

    return true;
}

void boss::update(world& w)
{
    if(_state == state::NONE)
    {
        return;
    }

    ++_timer;

    if(_flash && --_flash == 0)
    {
        _set_flash(false);
    }

    if(_state == state::ENTER)
    {
        bn::fixed dx = (_position.x() - _home_x) / 24;

        if(dx < bn::fixed(0.3))
        {
            _position.set_x(_home_x);
            _state = state::FIGHT;
            _timer = 0;
        }
        else
        {
            _position.set_x(_position.x() - dx);
        }
    }
    else if(_state == state::FIGHT)
    {
        switch(_id)
        {

        case boss_id::WARDEN:
            _update_warden(w);
            break;

        case boss_id::HIVE:
            _update_hive(w);
            break;

        default:
            _update_overmind(w);
            break;
        }
    }
    else
    {
        _update_dying(w);

        if(_state == state::NONE)
        {
            return;
        }
    }

    if(_id == boss_id::OVERMIND)
    {
        _update_pods(w);
    }

    _sync_sprites(w);
}

void boss::_update_warden(world& w)
{
    bn::fixed speed = bn::fixed(1) + _phase;
    _position.set_y(direction(_timer * (180 + _phase * 60), 34 + _phase * 6).y());

    if(_phase == 2)
    {
        _position.set_x(_home_x + direction(_timer * 256, 14).x());
    }

    if(_open_frames)
    {
        --_open_frames;
    }

    if(--_attack_timer > 0)
    {
        // Phase 3 keeps a rotating stream going between volleys.
        if(_phase == 2 && (_timer % 6) == 0)
        {
            w.bullets.fire(w, bullet_kind::SMALL, _muzzle(), direction(_timer * 1400, bn::fixed(1.4)));
        }

        return;
    }

    ++_cycle;
    _open_frames = 24;

    switch(_phase)
    {

    case 0:
        w.bullets.fire_fan(w, bullet_kind::BIG, _muzzle(), 3, bn::fixed(1.6), degrees(20));
        _attack_timer = 70;

        if((_cycle % 3) == 0)
        {
            w.foes.spawn(w, enemy_kind::DART, _position.x() + 8, _position.y() - 12, -40, 0);
            w.foes.spawn(w, enemy_kind::DART, _position.x() + 8, _position.y() + 12, 40, 0);
        }
        break;

    case 1:
        if(_cycle & 1)
        {
            w.bullets.fire_ring(w, bullet_kind::SMALL, _muzzle(), 10, bn::fixed(1.3), _cycle * 3000);
        }
        else
        {
            w.bullets.fire_aimed(w, bullet_kind::NEEDLE, _muzzle(), speed + 1, 0);
            w.bullets.fire_aimed(w, bullet_kind::NEEDLE, _muzzle() + bn::fixed_point(0, -10), speed + 1, 0);
            w.bullets.fire_aimed(w, bullet_kind::NEEDLE, _muzzle() + bn::fixed_point(0, 10), speed + 1, 0);
        }
        _attack_timer = 55;
        break;

    default:
        w.bullets.fire_fan(w, bullet_kind::BIG, _muzzle(), 5, bn::fixed(1.8), degrees(16));
        _attack_timer = 75;

        if((_cycle % 3) == 0)
        {
            w.foes.spawn(w, enemy_kind::DART, _position.x() + 8, _position.y() - 12, -60, 0);
            w.foes.spawn(w, enemy_kind::DART, _position.x() + 8, _position.y() + 12, 60, 0);
        }
        break;
    }
}

void boss::_update_hive(world& w)
{
    const bn::fixed_point& target = w.ship.position();

    if(_phase == 1)
    {
        // Ram attack cycle: align with the player, telegraph, dash left, return, ring burst.
        int t = _timer % 220;

        if(t < 60)
        {
            _position.set_y(_position.y() + (target.y() - _position.y()) / 16);
        }
        else if(t < 80)
        {
            _open_frames = 2;
            _position.set_x(_home_x + ((t & 2) ? 2 : -2));
        }
        else if(t < 120)
        {
            _position.set_x(_position.x() - 5);

            if(_position.x() < -70)
            {
                _position.set_x(-70);
            }
        }
        else if(t < 170)
        {
            _position.set_x(_position.x() + (_home_x - _position.x()) / 10);

            if(t == 169)
            {
                w.bullets.fire_ring(w, bullet_kind::BIG, _position, 12, bn::fixed(1.4), _timer * 97);
            }
        }
        else
        {
            _position.set_x(_home_x);
        }

        if(t < 60 && (_timer % 10) == 0)
        {
            w.bullets.fire(w, bullet_kind::SMALL, _position, direction(_timer * 900, bn::fixed(1.2)));
        }

        return;
    }

    // Phases 0 and 2: hover and fire rotating spiral arms.
    _position.set_x(_position.x() + (_home_x - _position.x()) / 8);
    _position.set_y(direction(_timer * 150, 40).y());

    int arms = _phase == 0 ? 2 : 3;
    int period = _phase == 0 ? 7 : 5;

    if((_timer % period) == 0)
    {
        int base = _timer * (_phase == 0 ? 700 : -800);

        for(int arm = 0; arm < arms; ++arm)
        {
            w.bullets.fire(w, bullet_kind::SMALL, _position,
                           direction(base + arm * (angle_turn / arms), bn::fixed(1.3) + bn::fixed(0.2) * _phase));
        }
    }

    if(_phase == 2 && (_timer % 150) == 0)
    {
        w.foes.spawn(w, enemy_kind::MINE, _position.x() - 10, _position.y() - 20, 0, 0);
        w.foes.spawn(w, enemy_kind::MINE, _position.x() - 10, _position.y() + 20, 0, 0);
    }

    if(_phase == 0 && (_timer % 120) == 60)
    {
        w.bullets.fire_fan(w, bullet_kind::BIG, _position, 3, bn::fixed(1.6), degrees(24));
    }
}

void boss::_update_overmind(world& w)
{
    int amplitude = _phase == 2 ? 30 : 18;
    _position.set_y(direction(_timer * (140 + _phase * 50), amplitude).y());

    switch(_phase)
    {

    case 0:
        if((_timer % 130) == 65)
        {
            w.bullets.fire_fan(w, bullet_kind::BIG, _muzzle(), 5, bn::fixed(1.5), degrees(15));
        }
        break;

    case 1:
        {
            // Needle waves from the eye, then a big ring.
            int t = _timer % 150;

            if(t < 48 && (t % 4) == 0)
            {
                bn::fixed vy = direction(t * 2048, bn::fixed(1.2)).y();
                w.bullets.fire(w, bullet_kind::NEEDLE, _muzzle(), bn::fixed_point(-3, vy));
            }
            else if(t == 90)
            {
                w.bullets.fire_ring(w, bullet_kind::BIG, _muzzle(), 14, bn::fixed(1.3), _timer * 211);
            }
        }
        break;

    default:
        if((_timer % 4) == 0)
        {
            int base = _timer * 900;
            w.bullets.fire(w, bullet_kind::SMALL, _muzzle(), direction(base, bn::fixed(1.4)));
            w.bullets.fire(w, bullet_kind::SMALL, _muzzle(), direction(base + angle_left, bn::fixed(1.4)));
        }

        if((_timer % 70) == 0)
        {
            w.bullets.fire_fan(w, bullet_kind::BIG, _muzzle(), 3, bn::fixed(2), degrees(14));
        }

        if((_timer % 220) == 110)
        {
            w.foes.spawn(w, enemy_kind::MINE, _position.x() + 20, -60, 0, 0);
            w.foes.spawn(w, enemy_kind::MINE, _position.x() + 20, 60, 0, 0);
        }
        break;
    }
}

void boss::_update_pods(world& w)
{
    for(int index = 0; index < 2; ++index)
    {
        pod& p = _pods[index];

        if(! p.alive)
        {
            continue;
        }

        int side = index == 0 ? -1 : 1;
        bn::fixed bob = direction(_timer * 400 + index * 20000, 6).y();
        p.position = bn::fixed_point(_position.x() - 14 + bob, _position.y() + side * 44 + bob);

        if(p.flash && --p.flash == 0 && p.sprite)
        {
            p.sprite->set_palette(bn::sprite_items::boss_pod.palette_item());
        }

        if(_state == state::FIGHT && ((_timer + index * 30) % 60) == 0 && w.ship.alive())
        {
            w.bullets.fire_aimed(w, bullet_kind::SMALL, p.position, bn::fixed(1.8), 0);
        }

        set_frame(p.sprite, bn::sprite_items::boss_pod, p.frame, (_timer >> 4) & 1);
    }
}

void boss::_update_dying(world& w)
{
    const boss_info& info = boss_infos[int(_id)];
    _position.set_y(_position.y() + bn::fixed(0.15));

    switch(_timer)
    {

    case 1:
        w.bullets.cancel_all(w);
        break;

    case 2:
        w.foes.destroy_all(w);
        break;

    case 3:
        w.add_score(info.score);
        break;

    default:
        break;
    }

    if((_timer % 7) == 0)
    {
        bn::fixed_point offset(w.rng.get_int(-28, 28), w.rng.get_int(-24, 24));
        w.fx.explosion_small(w, _position + offset);
        audio::play(audio::sfx::EXPLODE);
    }

    if((_timer % 30) == 0)
    {
        bn::fixed_point offset(w.rng.get_int(-20, 20), w.rng.get_int(-16, 16));
        w.fx.explosion_big(w, _position + offset);
        w.shake(15, 3);
        audio::play(audio::sfx::EXPLODE_BIG);

        if(_id == boss_id::OVERMIND)
        {
            w.fx.explosion_big(w, _position + bn::fixed_point(overmind_rear_offset, 0) + offset);
        }
    }

    // Flicker while breaking apart.
    if(_body)
    {
        _body->set_visible((_timer & 2) == 0 || _timer < 30);
    }

    if(_timer >= info.dying_frames)
    {
        w.fx.explosion_big(w, _position);
        w.fx.explosion_big(w, _position + bn::fixed_point(-16, -12), 4);
        w.fx.explosion_big(w, _position + bn::fixed_point(16, 12), 8);
        w.shake(40, 4);
        audio::play(audio::sfx::EXPLODE_BIG);
        _body.reset();
        _rear.reset();

        for(pod& p : _pods)
        {
            p.alive = false;
            p.sprite.reset();
        }

        _state = state::NONE;
        w.notify_boss_defeated();
    }
}

void boss::_sync_sprites(world& w)
{
    (void) w;

    int frame = 0;

    switch(_id)
    {

    case boss_id::WARDEN:
        frame = _open_frames ? 1 : 0;
        break;

    case boss_id::HIVE:
        frame = _open_frames || _phase == 2 ? 2 : (_timer >> 3) & 1;
        break;

    default:
        frame = _phase;
        break;
    }

    set_frame(_body, *boss_infos[int(_id)].body, _body_frame, frame);

    if(_body)
    {
        _body->set_position(_position);
    }

    if(_rear)
    {
        set_frame(_rear, bn::sprite_items::boss_overmind_rear, _rear_frame, (_timer >> 2) & 1);
        _rear->set_position(_position + bn::fixed_point(overmind_rear_offset, 0));
    }

    for(pod& p : _pods)
    {
        if(p.sprite)
        {
            p.sprite->set_position(p.position);
        }
    }
}

}
