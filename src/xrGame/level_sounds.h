//---------------------------------------------------------------------------
#ifndef LevelSoundsH
#define LevelSoundsH
#pragma once

struct SStaticSound
{
    ref_sound m_Source;
    Ivector2 m_ActiveTime;
    Ivector2 m_PlayTime;
    Ivector2 m_PauseTime;
    u32 m_NextTime;
    u32 m_StopTime;
    Fvector m_Position;
    float m_Volume;
    float m_Freq;

public:
    void Load(IReader& F);
    void Update(u32 gt, u32 rt);
};

// music interface
struct SMusicTrack
{
#ifdef DEBUG
    shared_str m_DbgName;
#endif
    ref_sound m_SourceLeft;
    ref_sound m_SourceRight;
    ref_sound m_SourceStereo;
    Ivector2 m_ActiveTime;
    Ivector2 m_PauseTime;
    float m_Volume;

public:
    void Load(LPCSTR fn, LPCSTR params);
    BOOL in(u32 game_time);
    bool IsPlaying() const;
    void Play();
    void Stop();
    void SetVolume(float volume);
};

class CLevelSoundManager
{
    using StaticSoundsVec = xr_vector<SStaticSound>;
    StaticSoundsVec m_StaticSounds;
    using MusicTrackVec = xr_vector<SMusicTrack>;
    MusicTrackVec m_MusicTracks;
    u32 m_NextTrackTime;
    int m_CurrentTrack;

public:
    CLevelSoundManager();
    void Load();
    void Unload();
    void Update();
};

#endif
