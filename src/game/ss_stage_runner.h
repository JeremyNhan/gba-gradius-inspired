#ifndef SS_STAGE_RUNNER_H
#define SS_STAGE_RUNNER_H

namespace ss
{

class world;
struct stage_event;

/**
 * Plays a stage's event timeline: every frame, all events whose frame stamp has been reached are
 * executed in order. Events are sorted by frame in the data (ss_stage_data.cpp).
 */
class stage_runner
{

public:
    void update(world& w);

    /// Debug/test helper: jumps the timeline to the boss warning.
    void skip_to_boss(world& w);

    [[nodiscard]] bool boss_started() const
    {
        return _boss_started;
    }

private:
    int _next = 0;
    bool _boss_started = false;

    void _run(world& w, const stage_event& event);
};

}

#endif
