#include "ss_stage_runner.h"

#include "ss_audio.h"
#include "ss_world.h"

namespace ss
{

void stage_runner::update(world& w)
{
    const bn::span<const stage_event>& events = w.stage.events;

    while(_next < events.size() && events[_next].frame <= w.stage_frame)
    {
        _run(w, events[_next]);
        ++_next;
    }
}

void stage_runner::skip_to_boss(world& w)
{
    const bn::span<const stage_event>& events = w.stage.events;

    for(int index = _next; index < events.size(); ++index)
    {
        if(events[index].type == event_type::WARNING || events[index].type == event_type::BOSS)
        {
            _next = index;
            w.stage_frame = events[index].frame;
            return;
        }
    }
}

void stage_runner::_run(world& w, const stage_event& event)
{
    switch(event.type)
    {

    case event_type::SPAWN:
        {
            enemy_kind kind = enemy_kind(event.a);
            bn::fixed x = (event.d & flag_from_left) ? -136 : 136;
            bn::fixed y = event.b;

            if(kind == enemy_kind::TURRET)
            {
                // Turrets sit on the terrain surface where they scroll in.
                y = (event.d & flag_ceiling) ? w.ground.ceiling_y(x, w.scroll_x) + 8 :
                                               w.ground.floor_y(x, w.scroll_x) - 8;
            }

            w.foes.spawn(w, kind, x, y, event.c, event.d);
        }
        break;

    case event_type::FORMATION:
        w.foes.spawn_formation(w, formation_type(event.a), event.b, event.c, event.d);
        break;

    case event_type::SCROLL_SPEED:
        w.scroll_speed = bn::fixed(event.b) / 100;
        break;

    case event_type::WARNING:
        w.display.show_warning();
        audio::play(audio::sfx::WARNING);
        audio::stop_music();
        break;

    case event_type::BOSS:
        _boss_started = true;
        audio::play_music(audio::music::BOSS);
        w.big_boss.start(w, boss_id(event.a));
        break;

    default:
        break;
    }
}

}
