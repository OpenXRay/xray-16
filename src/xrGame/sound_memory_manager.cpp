////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_memory_manager.cpp
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Sound memory manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "sound_memory_manager.h"
#include "memory_manager.h"
#include "hit_memory_manager.h"
#include "visual_memory_manager.h"
#include "enemy_manager.h"
#include "memory_space_impl.h"
#include "CustomMonster.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrAICore/Navigation/level_graph.h"
#include "sound_user_data_visitor.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "xrEngine/profiler.h"
#include "client_spawn_manager.h"
#include "memory_manager.h"
#include "xrEngine/IGame_Persistent.h"

#ifndef MASTER_GOLD
#include "Actor.h"
#include "ai_debug.h"
#endif // MASTER_GOLD

#define SILENCE
//#define SAVE_OWN_SOUNDS
//#define SAVE_OWN_ITEM_SOUNDS
#define SAVE_NON_ALIVE_OBJECT_SOUNDS
#define SAVE_FRIEND_ITEM_SOUNDS
#define SAVE_FRIEND_SOUNDS
//#define SAVE_VISIBLE_OBJECT_SOUNDS

const float COMBAT_SOUND_PERCEIVE_RADIUS_SQR = _sqr(5.f);

CSoundMemoryManager::~CSoundMemoryManager()
{
    clear_delayed_objects();
#ifdef USE_SELECTED_SOUND
    xr_delete(m_selected_sound);
#endif
}

void CSoundMemoryManager::Load(LPCSTR section) {}
void CSoundMemoryManager::reinit()
{
    m_sounds = 0;
    m_priorities.clear();
    m_last_sound_time = 0;
    m_sound_threshold = m_min_sound_threshold;
    VERIFY(_valid(m_sound_threshold));
#ifdef USE_SELECTED_SOUND
    xr_delete(m_selected_sound);
#endif
}

void CSoundMemoryManager::reload(LPCSTR section)
{
    m_max_sound_count = READ_IF_EXISTS(pSettings, r_s32, section, "DynamicSoundsCount", 1);
    m_min_sound_threshold = READ_IF_EXISTS(pSettings, r_float, section, "sound_threshold", 0.05f);
    m_self_sound_factor = READ_IF_EXISTS(pSettings, r_float, section, "self_sound_factor", 0.f);
    m_sound_decrease_quant = READ_IF_EXISTS(pSettings, r_u32, section, "self_decrease_quant", 250);
    m_decrease_factor = READ_IF_EXISTS(pSettings, r_float, section, "self_decrease_factor", .95f);

    LPCSTR sound_perceive_section = READ_IF_EXISTS(pSettings, r_string, section, "sound_perceive_section", section);
    m_weapon_factor = READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "weapon", 10.f);
    m_item_factor = READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "item", 1.f);
    m_npc_factor = READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "npc", 1.f);
    m_anomaly_factor = READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "anomaly", 1.f);
    m_world_factor = READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "world", 1.f);
}

IC void CSoundMemoryManager::update_sound_threshold()
{
    VERIFY(_valid(m_self_sound_factor));
    VERIFY(_valid(m_sound_threshold));
    VERIFY(_valid(m_min_sound_threshold));
    VERIFY(!fis_zero(m_decrease_factor));
    VERIFY(m_sound_decrease_quant);
    // t = max(t*f^((tc - tl)/tq),min_threshold)
    m_sound_threshold =
        _max(m_self_sound_factor * m_sound_threshold * exp(float(Device.dwTimeGlobal - m_last_sound_time) /
                                                           float(m_sound_decrease_quant) * log(m_decrease_factor)),
            m_min_sound_threshold);
    VERIFY(_valid(m_sound_threshold));
}

IC u32 CSoundMemoryManager::priority(const MemorySpace::CSoundObject& sound) const
{
    u32 priority = u32(-1);
    xr_map<ESoundTypes, u32>::const_iterator I = m_priorities.begin();
    xr_map<ESoundTypes, u32>::const_iterator E = m_priorities.end();
    for (; I != E; ++I)
        if (((*I).second < priority) && ((*I).first & sound.m_sound_type) == (*I).first)
            priority = (*I).second;
    return (priority);
}

void CSoundMemoryManager::enable(const IGameObject* object, bool enable)
{
    xr_vector<CSoundObject>::iterator J = std::find(m_sounds->begin(), m_sounds->end(), object_id(object));
    if (J == m_sounds->end())
        return;
    (*J).m_enabled = enable;
}

IC bool is_sound_type(int s, const ESoundTypes& t) { return ((s & t) == t); }
void CSoundMemoryManager::feel_sound_new(
    IGameObject* object, int sound_type, CSound_UserDataPtr user_data, const Fvector& position, float sound_power)
{
#ifndef MASTER_GOLD
    if (object && smart_cast<CActor*>(object) && psAI_Flags.test(aiIgnoreActor))
        return;
#endif // MASTER_GOLD

    VERIFY(_valid(sound_power));
    if (!m_sounds)
        return;

    if (user_data)
        user_data->accept(m_visitor);

    IGameObject* self = m_object;
    VERIFY(self);
#ifndef SILENCE
    Msg("%s (%d) - sound type %x from %s at %d in (%.2f,%.2f,%.2f) with power %.2f", *self->cName(),
        Device.dwTimeGlobal, sound_type, object ? *object->cName() : "world", Device.dwTimeGlobal, position.x,
        position.y, position.z, sound_power);
#endif

    VERIFY(_valid(m_sound_threshold));
    m_object->sound_callback(object, sound_type, position, sound_power);
    VERIFY(_valid(m_sound_threshold));

    update_sound_threshold();

    CEntityAlive* entity_alive = m_object;
    if (!entity_alive->g_Alive())
        return;

    VERIFY(_valid(sound_power));
    if (is_sound_type(sound_type, SOUND_TYPE_WEAPON))
        sound_power *= m_weapon_factor;

    VERIFY(_valid(sound_power));
    if (is_sound_type(sound_type, SOUND_TYPE_ITEM))
        sound_power *= m_item_factor;

    VERIFY(_valid(sound_power));
    if (is_sound_type(sound_type, SOUND_TYPE_MONSTER))
        sound_power *= m_npc_factor;

    VERIFY(_valid(sound_power));
    if (is_sound_type(sound_type, SOUND_TYPE_ANOMALY))
        sound_power *= m_anomaly_factor;

    VERIFY(_valid(sound_power));
    if (is_sound_type(sound_type, SOUND_TYPE_WORLD))
        sound_power *= m_world_factor;

    VERIFY(_valid(sound_power));
    if (sound_power >= m_sound_threshold)
    {
        if (is_sound_type(sound_type, SOUND_TYPE_WEAPON_SHOOTING))
        {
            // this is fake!
            CEntityAlive* _entity_alive = smart_cast<CEntityAlive*>(object);
            if (_entity_alive && (self->ID() != _entity_alive->ID()) &&
                (_entity_alive->g_Team() != entity_alive->g_Team()))
                m_object->memory().hit().add(_entity_alive);
        }
        if (!m_stalker || !m_stalker->memory().enemy().selected())
            add(object, sound_type, position, sound_power);
        else
        {
            if (object)
            {
                //				bool		is_shooting = is_sound_type(sound_type,SOUND_TYPE_WEAPON_SHOOTING);
                //				bool		is_colliding = is_sound_type(sound_type,SOUND_TYPE_WORLD_OBJECT_COLLIDING);
                //				bool		very_close = m_stalker->Position().distance_to_sqr(object->Position()) <=
                // COMBAT_SOUND_PERCEIVE_RADIUS_SQR;
                //				if (is_shooting || is_colliding || very_close)
                add(object, sound_type, position, sound_power);
            }
        }
    }

    m_last_sound_time = Device.dwTimeGlobal;
    VERIFY(_valid(m_sound_threshold));
    m_sound_threshold = _max(m_sound_threshold, sound_power);
    VERIFY(_valid(m_sound_threshold));
}

void CSoundMemoryManager::add(const CSoundObject& sound_object, bool check_for_existance)
{
    if (check_for_existance)
    {
        if (m_sounds->end() != std::find(m_sounds->begin(), m_sounds->end(), object_id(sound_object.m_object)))
            return;
    }

    VERIFY(m_max_sound_count);
    if (m_max_sound_count <= m_sounds->size())
    {
        xr_vector<CSoundObject>::iterator I =
            std::min_element(m_sounds->begin(), m_sounds->end(), SLevelTimePredicate<CGameObject>());
        VERIFY(m_sounds->end() != I);
        *I = sound_object;
    }
    else
        m_sounds->push_back(sound_object);
}

void CSoundMemoryManager::add(const IGameObject* object, int sound_type, const Fvector& position, float sound_power)
{
#ifndef SAVE_OWN_SOUNDS
    // we do not want to save our own sounds
    if (object && (m_object->ID() == object->ID()))
        return;
#endif

#ifndef SAVE_OWN_ITEM_SOUNDS
    // we do not want to save the sounds which was from the items we own
    if (object && object->H_Parent() && (object->H_Parent()->ID() == m_object->ID()))
        return;
#endif

#ifndef SAVE_NON_ALIVE_OBJECT_SOUNDS
    // we do not want to save sounds from the non-alive objects (?!)
    if (object && !m_object->memory().enemy().selected() && !smart_cast<const CEntityAlive*>(object))
        return;
#endif

#ifndef SAVE_FRIEND_ITEM_SOUNDS
    // we do not want to save sounds from the teammates items
    CEntityAlive* me = m_object;
    if (object && object->H_Parent() &&
        (me->tfGetRelationType(smart_cast<const CEntityAlive*>(object->H_Parent())) == ALife::eRelationTypeFriend))
        return;
#endif

#ifndef SAVE_FRIEND_SOUNDS
    const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(object);
    // we do not want to save sounds from the teammates
    if (entity_alive && me && (me->tfGetRelationType(entity_alive) == ALife::eRelationTypeFriend))
        return;
#endif

#ifndef SAVE_VISIBLE_OBJECT_SOUNDS
#ifdef SAVE_FRIEND_SOUNDS
    const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(object);
#endif
    // we do not save sounds from the objects we see (?!)
    if (m_object->memory().visual().visible_now(entity_alive))
        return;
#endif

    const CGameObject* game_object = smart_cast<const CGameObject*>(object);
    if (!game_object && object)
        return;

    const CGameObject* self = m_object;

    xr_vector<CSoundObject>::iterator J = std::find(m_sounds->begin(), m_sounds->end(), object_id(object));
    if (m_sounds->end() == J)
    {
        CSoundObject sound_object;

        sound_object.fill(game_object, self, ESoundTypes(sound_type), sound_power,
            !m_stalker ? squad_mask_type(-1) : m_stalker->agent_manager().member().mask(m_stalker));
        if (!game_object)
            sound_object.m_object_params.m_position = position;
#ifdef USE_FIRST_GAME_TIME
        sound_object.m_first_game_time = Level().GetGameTime();
#endif
#ifdef USE_FIRST_LEVEL_TIME
        sound_object.m_first_level_time = Device.dwTimeGlobal;
#endif
        add(sound_object);
    }
    else
    {
        (*J).fill(game_object, self, ESoundTypes(sound_type), sound_power,
            (!m_stalker ? (*J).m_squad_mask.get() :
                          ((*J).m_squad_mask.get() | m_stalker->agent_manager().member().mask(m_stalker))));
        if (!game_object)
            (*J).m_object_params.m_position = position;
    }
}

struct CRemoveOfflinePredicate
{
    bool operator()(const CSoundObject& object) const
    {
        if (!object.m_object)
            return (false);

        return (!!object.m_object->H_Parent());
    }
};

void CSoundMemoryManager::update()
{
    START_PROFILE("Memory Manager/sounds::update")

    clear_delayed_objects();

    VERIFY(m_sounds);
    m_sounds->erase(std::remove_if(m_sounds->begin(), m_sounds->end(), CRemoveOfflinePredicate()), m_sounds->end());

#ifdef USE_SELECTED_SOUND
    xr_delete(m_selected_sound);
    u32 priority = u32(-1);
    xr_vector<CSoundObject>::const_iterator I = m_sounds->begin();
    xr_vector<CSoundObject>::const_iterator E = m_sounds->end();
    for (; I != E; ++I)
    {
        u32 cur_priority = this->priority(*I);
        if (cur_priority < priority)
        {
            m_selected_sound = new CSoundObject(*I);
            priority = cur_priority;
        }
    }
#endif

    STOP_PROFILE
}

struct CSoundObjectPredicate
{
    const IGameObject* m_object;

    CSoundObjectPredicate(const IGameObject* object) : m_object(object) {}
    bool operator()(const MemorySpace::CSoundObject& sound_object) const
    {
        if (!m_object)
            return (!sound_object.m_object);

        if (!sound_object.m_object)
            return (false);

        return (m_object->ID() == sound_object.m_object->ID());
    }
};

void CSoundMemoryManager::remove_links(IGameObject* object)
{
    VERIFY(m_sounds);
    SOUNDS::iterator I = std::find_if(m_sounds->begin(), m_sounds->end(), CSoundObjectPredicate(object));
    if (I != m_sounds->end())
        m_sounds->erase(I);

#ifdef USE_SELECTED_SOUND
    if (!m_selected_sound)
        return;

    if (!m_selected_sound->m_object)
        return;

    if (m_selected_sound->m_object->ID() != object->ID())
        return;

    xr_delete(m_selected_sound);
#endif
}

void CSoundMemoryManager::save(NET_Packet& packet) const
{
    if (!m_object->g_Alive())
        return;

    packet.w_u8((u8)objects().size());

    SOUNDS::const_iterator I = objects().begin();
    SOUNDS::const_iterator E = objects().end();
    for (; I != E; ++I)
    {
        packet.w_u16((*I).m_object ? (*I).m_object->ID() : ALife::_OBJECT_ID(-1));
        // object params
        packet.w_u32((*I).m_object_params.m_level_vertex_id);
        packet.w_vec3((*I).m_object_params.m_position);
#ifdef USE_ORIENTATION
        packet.w_float((*I).m_object_params.m_orientation.yaw);
        packet.w_float((*I).m_object_params.m_orientation.pitch);
        packet.w_float((*I).m_object_params.m_orientation.roll);
#endif // USE_ORIENTATION
        // self params
        packet.w_u32((*I).m_self_params.m_level_vertex_id);
        packet.w_vec3((*I).m_self_params.m_position);
#ifdef USE_ORIENTATION
        packet.w_float((*I).m_self_params.m_orientation.yaw);
        packet.w_float((*I).m_self_params.m_orientation.pitch);
        packet.w_float((*I).m_self_params.m_orientation.roll);
#endif // USE_ORIENTATION
#ifdef USE_LEVEL_TIME
        packet.w_u32((Device.dwTimeGlobal >= (*I).m_level_time) ? (Device.dwTimeGlobal - (*I).m_level_time) : 0);
#endif // USE_LAST_LEVEL_TIME
#ifdef USE_LEVEL_TIME
        packet.w_u32((Device.dwTimeGlobal >= (*I).m_level_time) ? (Device.dwTimeGlobal - (*I).m_last_level_time) : 0);
#endif // USE_LAST_LEVEL_TIME
#ifdef USE_FIRST_LEVEL_TIME
        packet.w_u32((Device.dwTimeGlobal >= (*I).m_level_time) ? (Device.dwTimeGlobal - (*I).m_first_level_time) : 0);
#endif // USE_FIRST_LEVEL_TIME
        packet.w_u32((*I).m_sound_type);
        packet.w_float((*I).m_power);
    }
}

void CSoundMemoryManager::load(IReader& packet)
{
    if (!m_object->g_Alive())
        return;

    typedef CClientSpawnManager::CALLBACK_TYPE CALLBACK_TYPE;
    CALLBACK_TYPE callback;
    callback.bind(&m_object->memory(), &CMemoryManager::on_requested_spawn);

    int count = packet.r_u8();
    for (int i = 0; i < count; ++i)
    {
        CDelayedSoundObject delayed_object;
        delayed_object.m_object_id = packet.r_u16();

        CSoundObject& object = delayed_object.m_sound_object;
        if (delayed_object.m_object_id != ALife::_OBJECT_ID(-1))
            object.m_object = smart_cast<CGameObject*>(Level().Objects.net_Find(delayed_object.m_object_id));
        else
            object.m_object = 0;

        // object params
        object.m_object_params.m_level_vertex_id = packet.r_u32();
        packet.r_fvector3(object.m_object_params.m_position);
#ifdef USE_ORIENTATION
        packet.r_float(object.m_object_params.m_orientation.yaw);
        packet.r_float(object.m_object_params.m_orientation.pitch);
        packet.r_float(object.m_object_params.m_orientation.roll);
#endif
        // self params
        object.m_self_params.m_level_vertex_id = packet.r_u32();
        packet.r_fvector3(object.m_self_params.m_position);
#ifdef USE_ORIENTATION
        packet.r_float(object.m_self_params.m_orientation.yaw);
        packet.r_float(object.m_self_params.m_orientation.pitch);
        packet.r_float(object.m_self_params.m_orientation.roll);
#endif
#ifdef USE_LEVEL_TIME
        VERIFY(Device.dwTimeGlobal >= object.m_level_time);
        object.m_level_time = packet.r_u32();
        object.m_level_time = Device.dwTimeGlobal - object.m_level_time;
#endif // USE_LEVEL_TIME
#ifdef USE_LAST_LEVEL_TIME
        VERIFY(Device.dwTimeGlobal >= object.m_last_level_time);
        object.m_last_level_time = packet.r_u32();
        object.m_last_level_time = Device.dwTimeGlobal - object.m_last_level_time;
#endif // USE_LAST_LEVEL_TIME
#ifdef USE_FIRST_LEVEL_TIME
        VERIFY(Device.dwTimeGlobal >= (*I).m_first_level_time);
        object.m_first_level_time = packet.r_u32();
        object.m_first_level_time = Device.dwTimeGlobal - object.m_first_level_time;
#endif // USE_FIRST_LEVEL_TIME
        object.m_sound_type = (ESoundTypes)packet.r_u32();
        object.m_power = packet.r_float();

        if (object.m_object || (delayed_object.m_object_id == ALife::_OBJECT_ID(-1)))
        {
            add(object, true);
            continue;
        }

        m_delayed_objects.push_back(delayed_object);

        const CClientSpawnManager::CSpawnCallback* spawn_callback =
            Level().client_spawn_manager().callback(delayed_object.m_object_id, m_object->ID());
        if (!spawn_callback || !spawn_callback->m_object_callback)
            if (!GEnv.isDedicatedServer)
                Level().client_spawn_manager().add(delayed_object.m_object_id, m_object->ID(), callback);
#ifdef DEBUG
            else
            {
                if (spawn_callback && spawn_callback->m_object_callback)
                {
                    VERIFY(spawn_callback->m_object_callback == callback);
                }
            }
#endif // DEBUG
    }
}

void CSoundMemoryManager::clear_delayed_objects()
{
    if (m_delayed_objects.empty())
        return;

    CClientSpawnManager& manager = Level().client_spawn_manager();
    DELAYED_SOUND_OBJECTS::const_iterator I = m_delayed_objects.begin();
    DELAYED_SOUND_OBJECTS::const_iterator E = m_delayed_objects.end();
    for (; I != E; ++I)
        if (manager.callback((*I).m_object_id, m_object->ID()))
            manager.remove((*I).m_object_id, m_object->ID());

    m_delayed_objects.clear();
}

void CSoundMemoryManager::on_requested_spawn(IGameObject* object)
{
    DELAYED_SOUND_OBJECTS::iterator I = m_delayed_objects.begin();
    DELAYED_SOUND_OBJECTS::iterator E = m_delayed_objects.end();
    for (; I != E; ++I)
    {
        if ((*I).m_object_id != object->ID())
            continue;

        if (m_object->g_Alive())
        {
            (*I).m_sound_object.m_object = smart_cast<CGameObject*>(object);
            VERIFY((*I).m_sound_object.m_object);
            add((*I).m_sound_object, true);
        }

        m_delayed_objects.erase(I);
        return;
    }
}
