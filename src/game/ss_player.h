#ifndef SS_PLAYER_H
#define SS_PLAYER_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_collision.h"
#include "ss_session.h"

namespace ss
{

class world;

class player
{

public:
    explicit player(world& w);

    void update(world& w);

    /// Called when something lethal touches the player. Returns true if the hit was absorbed
    /// (shield / invulnerability) and false if the ship was destroyed.
    bool hit(world& w);

    [[nodiscard]] bool alive() const
    {
        return _state == state::FLYING;
    }

    [[nodiscard]] bool vulnerable() const
    {
        return _state == state::FLYING && _invulnerable_frames == 0;
    }

    [[nodiscard]] const bn::fixed_point& position() const
    {
        return _position;
    }

    /// Small core hitbox (classic shmup: much smaller than the sprite).
    [[nodiscard]] hitbox core_hitbox() const
    {
        return make_hitbox(_position, 2, 2);
    }

    /// Larger box used for collecting power-ups.
    [[nodiscard]] hitbox pickup_hitbox() const
    {
        return make_hitbox(_position, 9, 7);
    }

    [[nodiscard]] bool waiting_to_respawn() const
    {
        return _state == state::DEAD;
    }

    /// Stage clear: stop shooting and fly out to the right.
    void start_outro()
    {
        if(_state == state::FLYING)
        {
            _state = state::OUTRO;
        }
    }

    [[nodiscard]] bool outro_done() const
    {
        return _state == state::OUTRO && _position.x() > 140;
    }

    void refresh_shield_sprite(world& w);

private:
    enum class state : unsigned char
    {
        FLYING,
        DEAD,
        OUTRO,
        GONE
    };

    bn::fixed_point _position;
    bn::optional<bn::sprite_ptr> _sprite;
    bn::optional<bn::sprite_ptr> _shield_sprite;
    bn::optional<bn::sprite_ptr> _charge_sprite;
    state _state = state::FLYING;
    int _frame = -1;
    int _bank = 0;              // -1 nose up, 0 level, 1 nose down (smoothed)
    int _bank_timer = 0;
    int _anim_counter = 0;
    int _invulnerable_frames = 0;
    int _dead_frames = 0;
    int _fire_cooldown = 0;
    int _missile_cooldown = 0;
    int _charge = 0;
    int _charge_frame = -1;

    void _move(world& w);
    void _fire(world& w);
    void _update_charge(world& w);
    void _update_sprites(world& w);
    void _respawn(world& w);
};

}

#endif
