#include "StdAfx.h"

#include "HudSound.h"

float psHUDSoundVolume = 1.0f;
float psHUDStepSoundVolume = 1.0f;

void InitHudSoundSettings()
{
    psHUDSoundVolume = pSettings->r_float("hud_sound", "hud_sound_vol_k");
    psHUDStepSoundVolume = pSettings->r_float("hud_sound", "hud_step_sound_vol_k");
}

void HUD_SOUND_ITEM::LoadSound(LPCSTR section, LPCSTR line, HUD_SOUND_ITEM& hud_snd, int type)
{
    hud_snd.m_activeSnd = nullptr;
    hud_snd.sounds.clear();

    string256 sound_line;
    xr_strcpy(sound_line, line);
    int k = 0;
    while (pSettings->line_exist(section, sound_line))
    {
        hud_snd.sounds.push_back(SSnd());
        SSnd& s = hud_snd.sounds.back();

        LoadSound(section, sound_line, s.snd, type, &s.volume, &s.delay);
        xr_sprintf(sound_line, "%s%d", line, ++k);
    } // while
}

void HUD_SOUND_ITEM::LoadSound(LPCSTR section, LPCSTR line, ref_sound& snd, int type, float* volume, float* delay)
{
    LPCSTR str = pSettings->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);
    snd.create(buf_str, st_Effect, type);

    if (volume != nullptr)
    {
        *volume = 1.f;
        if (count > 1)
        {
            _GetItem(str, 1, buf_str);
            if (xr_strlen(buf_str) > 0)
                *volume = (float)atof(buf_str);
        }
    }

    if (delay != nullptr)
    {
        *delay = 0;
        if (count > 2)
        {
            _GetItem(str, 2, buf_str);
            if (xr_strlen(buf_str) > 0)
                *delay = (float)atof(buf_str);
        }
    }
}

void HUD_SOUND_ITEM::DestroySound(HUD_SOUND_ITEM& hud_snd)
{
    for (auto& sound : hud_snd.sounds)
        sound.snd.destroy();

    hud_snd.sounds.clear();

    hud_snd.m_activeSnd = nullptr;
}

void HUD_SOUND_ITEM::PlaySound(
    HUD_SOUND_ITEM& hud_snd, const Fvector& position, const IGameObject* parent, bool b_hud_mode, bool looped, u8 index)
{
    if (hud_snd.sounds.empty())
        return;

    hud_snd.m_activeSnd = nullptr;
    StopSound(hud_snd);

    u32 flags = b_hud_mode ? sm_2D : 0;
    if (looped)
        flags |= sm_Looped;
    //Alundaio: Sanity, don't allow PlaySound of index greater then the size, just play last index
    if (index == u8(-1))
        index = (u8)Random.randI(hud_snd.sounds.size());
    else if (index >= (u8)hud_snd.sounds.size())
        index = (u8)hud_snd.sounds.size()-1;

    hud_snd.m_activeSnd = &hud_snd.sounds[index];

    hud_snd.m_activeSnd->snd.play_at_pos(const_cast<IGameObject*>(parent),
        flags & sm_2D ? Fvector().set(0, 0, 0) : position, flags, hud_snd.m_activeSnd->delay);

    hud_snd.m_activeSnd->snd.set_volume(hud_snd.m_activeSnd->volume * (b_hud_mode ? psHUDSoundVolume : 1.0f));
}

void HUD_SOUND_ITEM::StopSound(HUD_SOUND_ITEM& hud_snd)
{
    for (auto& sound : hud_snd.sounds)
        sound.snd.stop();

    hud_snd.m_activeSnd = nullptr;
}

//----------------------------------------------------------
HUD_SOUND_COLLECTION::~HUD_SOUND_COLLECTION()
{
    for (auto& sound_item : m_sound_items)
    {
        HUD_SOUND_ITEM::StopSound(sound_item);
        HUD_SOUND_ITEM::DestroySound(sound_item);
    }

    m_sound_items.clear();
}

HUD_SOUND_ITEM* HUD_SOUND_COLLECTION::FindSoundItem(LPCSTR alias, bool b_assert)
{
    xr_vector<HUD_SOUND_ITEM>::iterator it = std::find(m_sound_items.begin(), m_sound_items.end(), alias);

    if (it != m_sound_items.end())
        return &*it;

    R_ASSERT3(!b_assert, "sound item not found in collection", alias);
    return nullptr;
}

void HUD_SOUND_COLLECTION::PlaySound(
    LPCSTR alias, const Fvector& position, const IGameObject* parent, bool hud_mode, bool looped, u8 index)
{
    for (auto& sound_item : m_sound_items)
        if (sound_item.m_b_exclusive)
            HUD_SOUND_ITEM::StopSound(sound_item);

    HUD_SOUND_ITEM* snd_item = FindSoundItem(alias, true);
    HUD_SOUND_ITEM::PlaySound(*snd_item, position, parent, hud_mode, looped, index);
}

void HUD_SOUND_COLLECTION::StopSound(LPCSTR alias)
{
    HUD_SOUND_ITEM* snd_item = FindSoundItem(alias, true);
    HUD_SOUND_ITEM::StopSound(*snd_item);
}

void HUD_SOUND_COLLECTION::SetPosition(LPCSTR alias, const Fvector& pos)
{
    HUD_SOUND_ITEM* snd_item = FindSoundItem(alias, true);
    if (snd_item->playing())
        snd_item->set_position(pos);
}

void HUD_SOUND_COLLECTION::StopAllSounds()
{
    for (auto& sound_item : m_sound_items)
        HUD_SOUND_ITEM::StopSound(sound_item);
}

void HUD_SOUND_COLLECTION::LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
    R_ASSERT(NULL == FindSoundItem(alias, false));
    m_sound_items.resize(m_sound_items.size() + 1);
    HUD_SOUND_ITEM& snd_item = m_sound_items.back();
    HUD_SOUND_ITEM::LoadSound(section, line, snd_item, type);
    snd_item.m_alias = alias;
    snd_item.m_b_exclusive = exclusive;
}

//Alundaio:
/*
    It's usage is to play a group of sounds HUD_SOUND_ITEMs as if they were a single layered entity. This is a achieved by
    wrapping the class around HUD_SOUND_COLLECTION and tagging them with the same alias. This way, when one for example
    sndShot is played, it will play all the sound items with the same alias.
*/
//----------------------------------------------------------
HUD_SOUND_COLLECTION_LAYERED::~HUD_SOUND_COLLECTION_LAYERED()
{
    for (auto& sound_item : m_sound_items)
        sound_item.~HUD_SOUND_COLLECTION();

    m_sound_items.clear();
}

void HUD_SOUND_COLLECTION_LAYERED::StopAllSounds()
{
    for (auto& sound_item : m_sound_items)
        sound_item.StopAllSounds();
}

void HUD_SOUND_COLLECTION_LAYERED::StopSound(pcstr alias)
{
    for (auto& sound_item : m_sound_items)
        if (sound_item.m_alias == alias)
            sound_item.StopSound(alias);
}

void HUD_SOUND_COLLECTION_LAYERED::SetPosition(pcstr alias, const Fvector& pos)
{
    for (auto& sound_item : m_sound_items)
        if (sound_item.m_alias == alias)
            sound_item.SetPosition(alias, pos);
}

void HUD_SOUND_COLLECTION_LAYERED::PlaySound(pcstr alias, const Fvector& position, const IGameObject* parent, bool hud_mode, bool looped, u8 index)
{
    for (auto& sound_item : m_sound_items)
        if (sound_item.m_alias == alias)
            sound_item.PlaySound(alias, position, parent, hud_mode, looped, index);
}


HUD_SOUND_ITEM* HUD_SOUND_COLLECTION_LAYERED::FindSoundItem(pcstr alias, bool b_assert)
{
    for (auto& sound_item : m_sound_items)
        if (sound_item.m_alias == alias)
            return sound_item.FindSoundItem(alias, b_assert);

    return nullptr;
}

void HUD_SOUND_COLLECTION_LAYERED::LoadSound(pcstr section, pcstr line, pcstr alias, bool exclusive, int type)
{
    pcstr str = pSettings->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);

    if (pSettings->section_exist(buf_str))
    {
        string256 sound_line;
        xr_strcpy(sound_line,"snd_1_layer");
        int k = 1;
        while (pSettings->line_exist(buf_str, sound_line))
        {
            m_sound_items.resize(m_sound_items.size() + 1);
            HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
            snd_item.LoadSound(buf_str, sound_line, alias, exclusive, type);
            snd_item.m_alias = alias;
            xr_sprintf(sound_line,"snd_%d_layer", ++k);
        }
    }
    else // For compatibility with normal HUD_SOUND_COLLECTION sounds
    {
        m_sound_items.resize(m_sound_items.size() + 1);
        HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
        snd_item.LoadSound(section, line, alias, exclusive, type);
        snd_item.m_alias = alias;
    }
}

void HUD_SOUND_COLLECTION_LAYERED::LoadSound(CInifile const *ini, pcstr section, pcstr line, pcstr alias, bool exclusive, int type)
{
    pcstr str = ini->r_string(section, line);
    string256 buf_str;

    int count = _GetItemCount(str);
    R_ASSERT(count);

    _GetItem(str, 0, buf_str);

    if (ini->section_exist(buf_str))
    {
        string256 sound_line;
        xr_strcpy(sound_line, "snd_1_layer");
        int k = 1;
        while (ini->line_exist(buf_str, sound_line))
        {
            m_sound_items.resize(m_sound_items.size() + 1);
            HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
            snd_item.LoadSound(buf_str, sound_line, alias, exclusive, type);
            snd_item.m_alias = alias;
            xr_sprintf(sound_line, "snd_%d_layer", ++k);
        }
    }
    else //For compatibility with normal HUD_SOUND_COLLECTION sounds
    {
        m_sound_items.resize(m_sound_items.size() + 1);
        HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
        snd_item.LoadSound(section, line, alias, exclusive, type);
        snd_item.m_alias = alias;
    }
}
//-Alundaio
