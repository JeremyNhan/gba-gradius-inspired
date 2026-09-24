#ifndef SS_BOSS_H
#define SS_BOSS_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_collision.h"
#include "ss_stage_data.h"

namespace ss
{

class world;

/**
 * Stage bosses. Behaviour is deterministic: attack timing depends only on the boss timer, its
 * HP-based phase (3 phases each) and the player position.
 */
class boss
{

public:
    void start(world& w, boss_id id);

    void update(world& w);

    [[nodiscard]] bool active() const
    {
        return _state != state::NONE;
    }

    [[nodiscard]] bool fighting() const
    {
        return _state == state::FIGHT;
    }

    /// Applies a projectile hit if the box touches a vulnerable part. Returns true if it hit.
    bool take_hit(world& w, const hitbox& box, int damage);

    /// True if the box touches the boss body (lethal to the player).
    [[nodiscard]] bool touches(const hitbox& box) const;

    /// Aim point for homing missiles.
    [[nodiscard]] bn::optional<bn::fixed_point> target_point() const;

    [[nodiscard]] int hp() const
    {
        return _hp;
    }

    [[nodiscard]] int hp_max() const
    {
        return _hp_max;
    }

    [[nodiscard]] const bn::fixed_point& position() const
    {
        return _position;
    }

    [[nodiscard]] int phase() const
    {
        return _phase;
    }

private:
    enum class state : unsigned char
    {
        NONE,
        ENTER,
        FIGHT,
        DYING
    };

    struct pod
    {
        bool alive = false;
        int hp = 0;
        int flash = 0;
        int frame = -1;
        bn::fixed_point position;
        bn::optional<bn::sprite_ptr> sprite;
    };

    state _state = state::NONE;
    boss_id _id = boss_id::WARDEN;
    int _hp = 0;
    int _hp_max = 0;
    int _phase = 0;
    int _timer = 0;
    int _attack_timer = 0;
    int _flash = 0;
    int _open_frames = 0;
    int _body_frame = -1;
    int _rear_frame = -1;
    int _cycle = 0;
    bn::fixed _home_x;
    bn::fixed_point _position;
    bn::fixed_point _velocity;
    bn::optional<bn::sprite_ptr> _body;
    bn::optional<bn::sprite_ptr> _rear;
    pod _pods[2];

    void _update_warden(world& w);
    void _update_hive(world& w);
    void _update_overmind(world& w);
    void _update_pods(world& w);
    void _update_dying(world& w);
    void _sync_sprites(world& w);
    void _set_flash(bool flash);
    [[nodiscard]] hitbox _core_box() const;
    [[nodiscard]] bn::fixed_point _muzzle() const;
};

}

#endif
