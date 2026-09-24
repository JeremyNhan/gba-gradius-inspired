#ifndef SS_ENEMIES_H
#define SS_ENEMIES_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_collision.h"
#include "ss_constants.h"
#include "ss_enemy_data.h"
#include "ss_pool.h"
#include "ss_stage_data.h"

namespace ss
{

class world;

struct enemy
{
    bool active = false;
    bool entered = false;           // has been fully on screen at least once
    enemy_kind kind = enemy_kind::DART;
    const enemy_def* def = nullptr;
    bn::fixed_point position;
    bn::fixed_point velocity;
    bn::fixed base_y;
    int hp = 0;
    int timer = 0;
    int fire_timer = 0;
    int param = 0;
    int phase = 0;
    int angle = 0;
    int burst = 0;
    int frame = -1;
    int flash = 0;
    short flags = 0;
    signed char formation = -1;
    bn::optional<bn::sprite_ptr> sprite;

    void clear()
    {
        active = false;
        sprite.reset();
    }

    [[nodiscard]] hitbox box() const;
};

class enemies
{

public:
    void spawn(world& w, enemy_kind kind, bn::fixed x, bn::fixed y, int param, short flags, int formation = -1);

    void spawn_formation(world& w, formation_type type, int y, int count, short flags);

    void update(world& w);

    /// Applies damage from a player projectile; destroys the enemy when its HP runs out.
    void damage(world& w, enemy& target, int amount);

    /// Removes an enemy with an explosion. by_player: award score/drops.
    void destroy(world& w, enemy& target, bool by_player);

    /// Closest on-screen enemy for homing missiles.
    [[nodiscard]] bn::optional<bn::fixed_point> nearest_target(const bn::fixed_point& from) const;

    pool<enemy, max_enemies>& items()
    {
        return _pool;
    }

    [[nodiscard]] int count() const
    {
        return _pool.count();
    }

    [[nodiscard]] int dropped() const
    {
        return _pool.dropped();
    }

    /// Destroys every enemy (used when a boss dies). No score is awarded.
    void destroy_all(world& w);

private:
    struct formation_info
    {
        bool used = false;
        bool carrier = false;
        int alive = 0;
        int killed = 0;
        int spawned = 0;
    };

    static constexpr int max_formations = 6;

    pool<enemy, max_enemies> _pool;
    formation_info _formations[max_formations];

    int _alloc_formation(bool carrier);
    void _leave(world& w, enemy& target);
    void _formation_member_gone(world& w, enemy& target, bool killed);
    void _move(world& w, enemy& e);
    void _fire(world& w, enemy& e);
    void _animate(world& w, enemy& e);
};

[[nodiscard]] const enemy_def& enemy_definition(enemy_kind kind);

}

#endif
