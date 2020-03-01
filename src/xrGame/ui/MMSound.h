#pragma once

class CUIXml;

class CMMSound
{
public:
    CMMSound();
    ~CMMSound();
    void Init(CUIXml& xml_doc, LPCSTR path);
    void whell_Play();
    void whell_Stop();
    void whell_Click();
    void whell_UpdateMoving(float frequency);

    void music_Play();
    void music_Stop();
    void music_Update();

    void all_Stop();

protected:
    IC bool check_file(LPCSTR fname);

    static constexpr size_t channels_count = 2;
    xr_array<ref_sound, channels_count> m_music;

    ref_sound m_whell;
    ref_sound m_whell_click;
    bool m_bRandom;
    xr_vector<xr_string> m_play_list;
};
