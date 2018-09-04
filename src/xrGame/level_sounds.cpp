#include "stdafx.h"
#pragma hdrstop

#include "Level.h"
#include "level_sounds.h"

//-----------------------------------------------------------------------------
// static level sounds
//-----------------------------------------------------------------------------
void SStaticSound::Load(IReader& F)
{
    R_ASSERT(F.find_chunk(0));
    xr_string wav_name;
    F.r_stringZ(wav_name);
    m_Source.create(wav_name.c_str(), st_Effect, sg_SourceType);
    F.r_fvector3(m_Position);
    m_Volume = F.r_float();
    m_Freq = F.r_float();
    m_ActiveTime.x = F.r_u32();
    m_ActiveTime.y = F.r_u32();
    m_PlayTime.x = F.r_u32();
    m_PlayTime.y = F.r_u32();
    m_PauseTime.x = F.r_u32();
    m_PauseTime.y = F.r_u32();
    m_NextTime = 0;
    m_StopTime = 0;
}

void SStaticSound::Update(u32 game_time, u32 global_time)
{
    if ((0 == m_ActiveTime.x) && (0 == m_ActiveTime.y) ||
        ((int(game_time) >= m_ActiveTime.x) && (int(game_time) < m_ActiveTime.y)))
    {
        if (0 == m_Source._feedback())
        {
            if ((0 == m_PauseTime.x) && (0 == m_PauseTime.y))
            {
                m_Source.play_at_pos(0, m_Position, sm_Looped);
                m_Source.set_volume(m_Volume);
                m_Source.set_frequency(m_Freq);
                m_StopTime = 0xFFFFFFFF;
            }
            else
            {
                if (global_time >= m_NextTime)
                {
                    bool bFullPlay = (0 == m_PlayTime.x) && (0 == m_PlayTime.y);
                    m_Source.play_at_pos(0, m_Position, bFullPlay ? 0 : sm_Looped);
                    m_Source.set_volume(m_Volume);
                    m_Source.set_frequency(m_Freq);
                    if (bFullPlay)
                    {
                        m_StopTime = 0xFFFFFFFF;
                        m_NextTime = global_time + iFloor(m_Source.get_length_sec() * 1000.0f) +
                            Random.randI(m_PauseTime.x, m_PauseTime.y);
                    }
                    else
                    {
                        m_StopTime = bFullPlay ? 0 : global_time + Random.randI(m_PlayTime.x, m_PlayTime.y);
                        m_NextTime = m_StopTime + Random.randI(m_PauseTime.x, m_PauseTime.y);
                    }
                }
            }
        }
        else
        {
            if (Device.dwTimeGlobal >= m_StopTime)
                m_Source.stop_deferred();
        }
    }
    else
    {
        if (0 != m_Source._feedback())
            m_Source.stop_deferred();
    }
}
//-----------------------------------------------------------------------------
// music tracks
//-----------------------------------------------------------------------------
void SMusicTrack::Load(LPCSTR fn, LPCSTR params)
{
#ifdef DEBUG
    m_DbgName = fn;
#endif
    m_SourceStereo.create(fn, st_Music, sg_Undefined);

    // parse params
    int cnt = _GetItemCount(params);
    VERIFY(cnt == 5);
    m_ActiveTime.set(0, 0);
    m_PauseTime.set(0, 0);
    m_Volume = 1.f;
    sscanf(params, "%d,%d,%f,%d,%d", &m_ActiveTime.x, &m_ActiveTime.y, &m_Volume, &m_PauseTime.x, &m_PauseTime.y);

    if (m_PauseTime.x == m_PauseTime.y)
        ++m_PauseTime.y;

    m_ActiveTime.mul(60 * 60 * 1000); // convert hour to ms
    m_PauseTime.mul(1000); // convert sec to ms
}

BOOL SMusicTrack::in(u32 game_time)
{
    // game_time -ms
    if (m_ActiveTime.x == 0 && m_ActiveTime.y)
        return TRUE;

    bool b_cross_midnight = (m_ActiveTime.y < m_ActiveTime.x);
    BOOL res = FALSE;

    if (!b_cross_midnight)
    {
        res = ((int(game_time) >= m_ActiveTime.x) && (int(game_time) < m_ActiveTime.y));
    }
    else
    {
        res = ((int(game_time) >= m_ActiveTime.x) || (int(game_time) <= m_ActiveTime.y));
    }
    return res;
}

void SMusicTrack::Play()
{
    m_SourceStereo.play_at_pos(0, Fvector().set(0.0f, 0.0f, 0.0f), sm_2D);
    SetVolume(1.0f);
}

BOOL SMusicTrack::IsPlaying()
{
    BOOL ret = (NULL != m_SourceStereo._feedback());
    return ret;
}

void SMusicTrack::SetVolume(float volume) { m_SourceStereo.set_volume(volume * m_Volume); }
void SMusicTrack::Stop() { m_SourceStereo.stop_deferred(); }

//-----------------------------------------------------------------------------
// level sound manager
//-----------------------------------------------------------------------------
CLevelSoundManager::CLevelSoundManager() : m_CurrentTrack(0) { m_NextTrackTime = 0; }

void CLevelSoundManager::Load()
{
    // static level sounds
    VERIFY(m_StaticSounds.empty());
    string_path fn;
    if (FS.exist(fn, "$level$", "level.snd_static"))
    {
        IReader* F = FS.r_open(fn);
        u32 chunk = 0;
        for (IReader* OBJ = F->open_chunk_iterator(chunk); OBJ; OBJ = F->open_chunk_iterator(chunk, OBJ))
        {
            m_StaticSounds.push_back(SStaticSound());
            m_StaticSounds.back().Load(*OBJ);
        }
        FS.r_close(F);
    }

    // music
    m_CurrentTrack = -1;

    CInifile& gameLtx = *pGameIni;

    if (gameLtx.section_exist(Level().name()))
    {
        if (gameLtx.line_exist(Level().name(), "music_tracks"))
        {
            LPCSTR music_sect = gameLtx.r_string(Level().name(), "music_tracks");
            if (music_sect && music_sect[0])
            {
#ifdef DEBUG
                Msg("- Loading music tracks from '%s'...", music_sect);
#endif // #ifdef DEBUG
                CInifile::Sect& S = gameLtx.r_section(music_sect);
                auto it = S.Data.cbegin(), end = S.Data.cend();
                m_MusicTracks.reserve(S.Data.size());
                for (; it != end; it++)
                {
                    m_MusicTracks.push_back(SMusicTrack());
                    m_MusicTracks.back().Load(*it->first, *it->second);
                }
            }
        }
    }
}

void CLevelSoundManager::Unload()
{
    // static sounds
    m_StaticSounds.clear();
    // music
    m_MusicTracks.clear();
}

void CLevelSoundManager::Update()
{
    if (Device.Paused())
        return;
    if (Device.dwPrecacheFrame != 0)
        return;
    // static sounds
    u32 game_time = Level().GetGameDayTimeMS();
    u32 engine_time = Device.dwTimeGlobal;

    for (u32 k = 0; k < m_StaticSounds.size(); ++k)
    {
        SStaticSound& s = m_StaticSounds[k];
        s.Update(game_time, engine_time);
    }

    // music track
    if (!m_MusicTracks.empty())
    {
        if (m_CurrentTrack < 0 && engine_time > m_NextTrackTime)
        {
            U32Vec indices;
            for (u32 k = 0; k < m_MusicTracks.size(); ++k)
            {
                SMusicTrack& T = m_MusicTracks[k];
                if (T.IsPlaying())
                    T.Stop();

                if (T.in(game_time))
                    indices.push_back(k);
                /*
                                if ((0==T.m_ActiveTime.x) && (0==T.m_ActiveTime.y)||
                                    ((int(game_time)>=T.m_ActiveTime.x)&&(int(game_time)<T.m_ActiveTime.y)))
                                    indices.push_back	(k);
                */
            }
            if (!indices.empty())
            {
                u32 idx = Random.randI(indices.size());
                m_CurrentTrack = indices[idx];
                SMusicTrack& T = m_MusicTracks[m_CurrentTrack];
                T.Play();
#ifdef DEBUG
                Log("- Play music track:", T.m_DbgName.c_str());
#endif
            }
            else
            {
                m_NextTrackTime = engine_time + 10000; // next check after 10 sec
            }
        }

        if (m_CurrentTrack >= 0)
        {
            SMusicTrack& T = m_MusicTracks[m_CurrentTrack];
            if (!T.IsPlaying())
            {
                m_CurrentTrack = -1;
                m_NextTrackTime = engine_time;

                if (!((0 == T.m_PauseTime.x) && (0 == T.m_PauseTime.y)))
                    m_NextTrackTime += Random.randI(T.m_PauseTime.x, T.m_PauseTime.y);
            }
        }
    }
}
