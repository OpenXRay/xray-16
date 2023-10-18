#include "stdafx.h"

#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Target.h"
#include "SoundRender_Source.h"

void CSoundRender_Core::i_start(CSoundRender_Emitter* E)
{
    R_ASSERT(E);

    // Search lowest-priority target
    float Ptest = E->priority();
    float Ptarget = flt_max;
    CSoundRender_Target* T = nullptr;
    for (auto Ttest : s_targets)
    {
        if (Ttest->priority < Ptarget)
        {
            T = Ttest;
            Ptarget = Ttest->priority;
        }
    }

    // Stop currently playing
    if (T->get_emitter())
        T->get_emitter()->cancel();

    // Associate
    E->target = T;
    E->target->start(E);
    T->priority = Ptest;
}

void CSoundRender_Core::i_stop(CSoundRender_Emitter* E)
{
    // Msg("- %10s : %3d[%1.4f] : %s", "i_stop", E->dbg_ID, E->priority(), E->source->fname);
    R_ASSERT(E);
    R_ASSERT(E == E->target->get_emitter());
    E->target->stop();
    E->target = nullptr;
}

void CSoundRender_Core::i_rewind(CSoundRender_Emitter* E)
{
    // Msg("- %10s : %3d[%1.4f] : %s", "i_rewind", E->dbg_ID, E->priority(), E->source->fname);
    R_ASSERT(E);
    R_ASSERT(E == E->target->get_emitter());
    E->target->rewind();
}

bool CSoundRender_Core::i_allow_play(CSoundRender_Emitter* E)
{
    // Search available target
    float Ptest = E->priority();
    return std::any_of(s_targets.begin(), s_targets.end(), [Ptest](CSoundRender_Target* target)
    {
        return target->priority < Ptest;
    });
}
