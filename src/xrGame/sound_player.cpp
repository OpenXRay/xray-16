////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_player.cpp
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Sound player
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "sound_player.h"
#include "xrScriptEngine/script_engine.hpp"
#include "ai/stalker/ai_stalker_space.h"
#include "ai_space.h"
#include "xrEngine/xr_object.h"
#include "xrCore/Animation/Bone.hpp"
#include "Include/xrRender/Kinematics.h"
#include "xrEngine/profiler.h"
#include "sound_collection_storage.h"
#include "Common/object_broker.h"

CSoundPlayer::CSoundPlayer(IGameObject* object)
{
    VERIFY(object);
    m_object = object;
    seed(u32(CPU::QPC() & 0xffffffff));
    m_sound_prefix = "";
}

CSoundPlayer::~CSoundPlayer() { clear(); }
void CSoundPlayer::clear()
{
    m_sounds.clear();

    xr_vector<CSoundSingle>::iterator I = m_playing_sounds.begin();
    xr_vector<CSoundSingle>::iterator E = m_playing_sounds.end();
    for (; I != E; ++I)
        (*I).destroy();

    m_playing_sounds.clear();

    m_sound_mask = 0;
}

void CSoundPlayer::reinit() {}
void CSoundPlayer::reload(LPCSTR section)
{
    VERIFY(m_playing_sounds.empty());
    clear();
    m_sound_prefix = "";
}

void CSoundPlayer::unload()
{
    remove_active_sounds(u32(-1));
    VERIFY(m_playing_sounds.empty());
}

u32 CSoundPlayer::add(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type,
    LPCSTR bone_name, CSound_UserDataPtr data)
{
    SOUND_COLLECTIONS::iterator I = m_sounds.find(internal_type);
    if (m_sounds.end() != I)
        return (0);

    CSoundCollectionParamsFull sound_params;
    sound_params.m_priority = priority;
    sound_params.m_synchro_mask = mask;
    sound_params.m_bone_name = bone_name;
    sound_params.m_sound_prefix = prefix;
    sound_params.m_sound_player_prefix = m_sound_prefix;
    sound_params.m_max_count = max_count;
    sound_params.m_type = type;
    sound_params.m_data = data;

    typedef CSoundCollectionStorage::SOUND_COLLECTION_PAIR SOUND_COLLECTION_PAIR;
    const SOUND_COLLECTION_PAIR& pair = sound_collection_storage().object(sound_params);
    VERIFY(pair.first == (CSoundCollectionParams&)sound_params);
    VERIFY(pair.second);
    m_sounds.insert(std::make_pair(internal_type, std::make_pair(sound_params, pair.second)));
    return (pair.second->m_sounds.size());
}

void CSoundPlayer::remove(u32 internal_type)
{
    SOUND_COLLECTIONS::iterator I = m_sounds.find(internal_type);
    VERIFY(m_sounds.end() != I);
    m_sounds.erase(I);
}

bool CSoundPlayer::check_sound_legacy(u32 internal_type) const
{
    SOUND_COLLECTIONS::const_iterator J = m_sounds.find(internal_type);
    if (m_sounds.end() == J)
    {
#ifdef DEBUG
        GEnv.ScriptEngine->script_log(LuaMessageType::Message,
            "Can't find sound with internal type %d (sound_script = %d)", internal_type,
            StalkerSpace::eStalkerSoundScript);
#endif
        return (false);
    }

    VERIFY(m_sounds.end() != J);
    const CSoundCollectionParamsFull& sound = (*J).second.first;
    if (sound.m_synchro_mask & m_sound_mask)
        return (false);

    xr_vector<CSoundSingle>::const_iterator I = m_playing_sounds.begin();
    xr_vector<CSoundSingle>::const_iterator E = m_playing_sounds.end();
    for (; I != E; ++I)
        if ((*I).m_synchro_mask & sound.m_synchro_mask)
            if ((*I).m_priority <= sound.m_priority)
                return (false);
    return (true);
}

void CSoundPlayer::update(float time_delta)
{
    START_PROFILE("Sound Player")
    remove_inappropriate_sounds(m_sound_mask);
    update_playing_sounds();
    STOP_PROFILE
}

void CSoundPlayer::remove_inappropriate_sounds(u32 sound_mask)
{
    m_playing_sounds.erase(
        std::remove_if(m_playing_sounds.begin(), m_playing_sounds.end(), CInappropriateSoundPredicate(sound_mask)),
        m_playing_sounds.end());
}

void CSoundPlayer::update_playing_sounds()
{
    xr_vector<CSoundSingle>::iterator I = m_playing_sounds.begin();
    xr_vector<CSoundSingle>::iterator E = m_playing_sounds.end();
    for (; I != E; ++I)
    {
        if ((*I).m_sound->_feedback())
            (*I).m_sound->_feedback()->set_position(compute_sound_point(*I));
        else if (!(*I).started() && (Device.dwTimeGlobal >= (*I).m_start_time))
            (*I).play_at_pos(m_object, compute_sound_point(*I));
    }
}

bool CSoundPlayer::need_bone_data() const
{
    xr_vector<CSoundSingle>::const_iterator I = m_playing_sounds.begin();
    xr_vector<CSoundSingle>::const_iterator E = m_playing_sounds.end();
    for (; I != E; ++I)
    {
        if ((*I).m_sound->_feedback())
            return (true);
        else if (!(*I).started() && (Device.dwTimeGlobal >= (*I).m_start_time))
            return (true);
    }
    return (false);
}

void CSoundPlayer::play(
    u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time, u32 id)
{
    if (!check_sound_legacy(internal_type))
        return;

    SOUND_COLLECTIONS::iterator I = m_sounds.find(internal_type);
    VERIFY(m_sounds.end() != I);
    CSoundCollectionParamsFull& sound = (*I).second.first;
    if ((*I).second.second->m_sounds.empty())
    {
#ifdef DEBUG
        Msg("- There are no sounds in sound collection \"%s\" with internal type %d (sound_script = %d)",
            *sound.m_sound_prefix, internal_type, StalkerSpace::eStalkerSoundScript);
#endif
        return;
    }

    remove_inappropriate_sounds(sound.m_synchro_mask);

    CSoundSingle sound_single;
    (CSoundParams&)sound_single = (CSoundParams&)sound;
    sound_single.m_bone_id = smart_cast<IKinematics*>(m_object->Visual())->LL_BoneID(sound.m_bone_name);
    R_ASSERT(sound_single.m_bone_id != BI_NONE);

    sound_single.m_sound = new ref_sound();
    /**
    sound_single.m_sound->clone	(
        *(*I).second.second->m_sounds[
            id == u32(-1)
            ?
            (*I).second.second->random(
                (*I).second.second->m_sounds.size()
            )
            :
            id
        ],
        st_Effect,
        sg_SourceType
    );
    /**/
    sound_single.m_sound->clone((*I).second.second->random(id), st_Effect, sg_SourceType);

    sound_single.m_sound->_p->g_object = m_object;
    sound_single.m_sound->_p->g_userdata = (*I).second.first.m_data;
    VERIFY(sound_single.m_sound->_handle());

    VERIFY(max_start_time >= min_start_time);
    VERIFY(max_stop_time >= min_stop_time);
    u32 random_time = 0;

    if (max_start_time)
        random_time = (max_start_time > min_start_time) ? random(max_start_time - min_start_time) + min_start_time :
                                                          max_start_time;

    sound_single.m_start_time = Device.dwTimeGlobal + random_time;

    random_time = 0;
    if (max_stop_time)
        random_time =
            (max_stop_time > min_stop_time) ? random(max_stop_time - min_stop_time) + min_stop_time : max_stop_time;

    sound_single.m_stop_time =
        sound_single.m_start_time + iFloor(sound_single.m_sound->get_length_sec() * 1000.0f) + random_time;
    m_playing_sounds.push_back(sound_single);

    if (Device.dwTimeGlobal >= m_playing_sounds.back().m_start_time)
        m_playing_sounds.back().play_at_pos(m_object, compute_sound_point(m_playing_sounds.back()));
}

IC Fvector CSoundPlayer::compute_sound_point(const CSoundSingle& sound)
{
    Fmatrix l_tMatrix;
    l_tMatrix.mul_43(m_object->XFORM(),
        smart_cast<IKinematics*>(m_object->Visual())->LL_GetBoneInstance(sound.m_bone_id).mTransform);
    return (l_tMatrix.c);
}

CSoundPlayer::CSoundCollection::CSoundCollection(const CSoundCollectionParams& params)
{
    m_last_sound_id = u32(-1);

    seed(u32(CPU::QPC() & 0xffffffff));
    m_sounds.clear();
    for (int j = 0, N = _GetItemCount(*params.m_sound_prefix); j < N; ++j)
    {
        string_path fn, s, temp;
        LPSTR S = (LPSTR)&s;
        _GetItem(*params.m_sound_prefix, j, temp);
        strconcat(sizeof(s), S, *params.m_sound_player_prefix, temp);
        if (FS.exist(fn, "$game_sounds$", S, ".ogg"))
        {
            ref_sound* temp = add(params.m_type, S);
            if (temp)
                m_sounds.push_back(temp);
        }
        for (u32 i = 0; i < params.m_max_count; ++i)
        {
            string256 name;
            xr_sprintf(name, "%s%d", S, i);
            if (FS.exist(fn, "$game_sounds$", name, ".ogg"))
            {
                ref_sound* temp = add(params.m_type, name);
                if (temp)
                    m_sounds.push_back(temp);
            }
        }
    }
#ifdef DEBUG
    if (m_sounds.empty())
        Msg("- There are no sounds with prefix %s", *params.m_sound_prefix);
#endif
}

CSoundPlayer::CSoundCollection::~CSoundCollection()
{
#ifdef DEBUG
    xr_vector<ref_sound*>::iterator I = m_sounds.begin();
    xr_vector<ref_sound*>::iterator E = m_sounds.end();
    for (; I != E; ++I)
    {
        VERIFY(*I);
        VERIFY(!(*I)->_feedback());
    }
#endif
    delete_data(m_sounds);
}

const ref_sound& CSoundPlayer::CSoundCollection::random(const u32& id)
{
    VERIFY(!m_sounds.empty());

    if (id != u32(-1))
    {
        m_last_sound_id = id;
        VERIFY(id < m_sounds.size());
        return (*m_sounds[id]);
    }

    if (m_sounds.size() <= 2)
    {
        m_last_sound_id = CRandom32::random(m_sounds.size());
        return (*m_sounds[m_last_sound_id]);
    }

    u32 result;
    do
    {
        result = CRandom32::random(m_sounds.size());
    } while (result == m_last_sound_id);

    m_last_sound_id = result;
    return (*m_sounds[result]);
}
