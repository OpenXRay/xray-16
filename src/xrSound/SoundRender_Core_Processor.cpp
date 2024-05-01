#include "stdafx.h"

#include "xrEngine/Engine.h"
#include "xrEngine/GameFont.h"
#include "xrEngine/PerformanceAlert.hpp"
#include "xrCDB/Intersect.hpp"
#include "SoundRender_Core.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Target.h"
#include "SoundRender_Source.h"

void CSoundRender_Core::update(const Fvector& P, const Fvector& D, const Fvector& N, const Fvector& R)
{
    ZoneScoped;

    if (0 == bReady)
        return;
    Stats.Update.Begin();
    isLocked = true;

    Timer.time_factor(psSoundTimeFactor); //--#SM+#--
    {
        const float new_tm = Timer.GetElapsed_sec();
        fTimer_Delta = new_tm - fTimer_Value;
        fTimer_Value = new_tm;

        const float new_tm_p = TimerPersistent.GetElapsed_sec();
        fTimerPersistent_Delta = new_tm_p - fTimerPersistent_Value;
        fTimerPersistent_Value = new_tm_p;
    }
    s_emitters_u++;

    const auto update_emitter = [this](CSoundRender_Emitter* emitter)
    {
        const bool ignore = emitter->bIgnoringTimeFactor;
        const float time = ignore ? fTimerPersistent_Value : fTimer_Value;
        const float delta = ignore ? fTimerPersistent_Delta : fTimer_Delta;
        emitter->update(time, delta);
        emitter->marker = s_emitters_u;
    };

    // Firstly update emitters, which are now being rendered
    for (CSoundRender_Target* T : s_targets)
    {
        if (CSoundRender_Emitter* E = T->get_emitter())
        {
            update_emitter(E);
        }
    }

    // Update emitters
    for (CSoundRender_Scene* scene : m_scenes)
    {
        auto& emitters = scene->get_emitters();
        for (u32 it = 0; it < emitters.size(); it++)
        {
            CSoundRender_Emitter* pEmitter = emitters[it];
            if (pEmitter->marker != s_emitters_u)
            {
                update_emitter(pEmitter);
            }
            if (!pEmitter->isPlaying())
            {
                // Stopped
                xr_delete(pEmitter);
                emitters.erase(emitters.begin() + it);
                it--;
            }
        }
    }

    // update listener
    update_listener(P, D, N, R, fTimer_Delta);

    // Events
    for (CSoundRender_Scene* scene : m_scenes)
        scene->update();

    isLocked = false;
    Stats.Update.End();
}

void CSoundRender_Core::render()
{
    ZoneScoped;

    isLocked = true;
    Stats.Render.Begin();

    for (CSoundRender_Target* T : s_targets)
    {
        if (T->get_emitter())
        {
            T->fill_parameters();
            if (T->get_Rendering())
                T->update();
            else
                T->render();
        }
    }

    Stats.Render.End();
    isLocked = false;
}

void CSoundRender_Core::statistic(CSound_stats* dest, CSound_stats_ext* ext)
{
    if (dest)
    {
        dest->_rendered = 0;
        dest->_simulated = 0;
        dest->_events = 0;

        for (auto T : s_targets)
        {
            if (T->get_emitter() && T->get_Rendering())
                dest->_rendered++;
        }

        for (CSoundRender_Scene* scene : m_scenes)
        {
            dest->_simulated += scene->get_emitters().size();
            dest->_events += scene->get_prev_events_count();
        }
    }
    if (ext)
    {
        for (CSoundRender_Scene* scene : m_scenes)
        {
            auto& emitters = scene->get_emitters();
            for (const auto emitter : emitters)
            {
                CSound_stats_ext::SItem item;
                item._3D = !emitter->b2D;
                item._rendered = !!emitter->target;
                item.params = emitter->p_source;
                item.volume = emitter->smooth_volume;
                if (emitter->owner_data)
                {
                    item.name = emitter->source()->file_name();
                    item.game_object = emitter->owner_data->g_object;
                    item.game_type = emitter->owner_data->g_type;
                    item.type = emitter->owner_data->s_type;
                }
                else
                {
                    item.game_object = nullptr;
                    item.game_type = 0;
                    item.type = st_Effect;
                }
                ext->append(item);
            }
        }
    }
}

void CSoundRender_Core::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    Stats.FrameEnd();
    CSound_stats sndStat;
    statistic(&sndStat, nullptr);
    font.OutNext("*** SOUND:    %2.2fms", Stats.Update.result);
    font.OutNext("    RENDER:   %2.2fms", Stats.Render.result);
    font.OutNext("Rendered:     %d", sndStat._rendered);
    font.OutNext("Simulated:    %d", sndStat._simulated);
    font.OutNext("Events:       %d", sndStat._events);
    Stats.FrameStart();
}
