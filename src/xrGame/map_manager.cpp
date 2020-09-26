#include "pch_script.h"
#include "map_manager.h"
#include "alife_registry_wrappers.h"
#include "InventoryOwner.h"
#include "Level.h"
#include "Actor.h"
#include "relation_registry.h"
#include "GameObject.h"
#include "map_location.h"
#include "GametaskManager.h"
#include "xrServer.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"

struct FindLocationBySpotID
{
    shared_str spot_id;
    u16 object_id;
    FindLocationBySpotID(const shared_str& s, u16 id) : spot_id(s), object_id(id) {}
    bool operator()(const SLocationKey& key) { return (spot_id == key.spot_type) && (object_id == key.object_id); }
};
struct FindLocationByID
{
    u16 object_id;
    FindLocationByID(u16 id) : object_id(id) {}
    bool operator()(const SLocationKey& key) { return (object_id == key.object_id); }
};

struct FindLocation
{
    CMapLocation* ml;
    FindLocation(CMapLocation* m) : ml(m) {}
    bool operator()(const SLocationKey& key) { return (ml == key.location); }
};

void SLocationKey::save(IWriter& stream)
{
    stream.w(&object_id, sizeof(object_id));

    stream.w_stringZ(spot_type);
    stream.w_u8(0);
    location->save(stream);
}

void SLocationKey::load(IReader& stream)
{
    stream.r(&object_id, sizeof(object_id));

    stream.r_stringZ(spot_type);
    stream.r_u8();

    location = xr_new<CMapLocation>(*spot_type, object_id);

    location->load(stream);
}

void SLocationKey::destroy() { delete_data(location); }
void CMapLocationRegistry::save(IWriter& stream)
{
    stream.w_u32((u32)objects().size());
    iterator I = m_objects.begin();
    iterator E = m_objects.end();
    for (; I != E; ++I)
    {
        u32 size = 0;
        vLocations::iterator i = (*I).second.begin();
        vLocations::iterator e = (*I).second.end();
        for (; i != e; ++i)
        {
            VERIFY((*i).location);
            if ((*i).location->Serializable())
                ++size;
        }
        stream.w(&(*I).first, sizeof((*I).first));
        stream.w_u32(size);
        i = (*I).second.begin();
        for (; i != e; ++i)
            if ((*i).location->Serializable())
                (*i).save(stream);
    }
}

CMapManager::CMapManager()
{
    m_locations_wrapper = xr_new<CMapLocationWrapper>();
    m_locations_wrapper->registry().init(1);
    m_locations = NULL;
}

CMapManager::~CMapManager()
{
    delete_data(m_deffered_destroy_queue); // from prev frame
    delete_data(m_locations_wrapper);
}

CMapLocation* CMapManager::AddMapLocation(const shared_str& spot_type, u16 id)
{
    CMapLocation* l = xr_new<CMapLocation>(spot_type.c_str(), id);
    Locations().push_back(SLocationKey(spot_type, id));
    Locations().back().location = l;
    if (IsGameTypeSingle() && g_actor)
        Actor()->callback(GameObject::eMapLocationAdded)(spot_type.c_str(), id);

    return l;
}

CMapLocation* CMapManager::AddRelationLocation(CInventoryOwner* pInvOwner)
{
    if (!Level().CurrentViewEntity())
        return NULL;

    ALife::ERelationType relation = ALife::eRelationTypeFriend;
    CInventoryOwner* pActor = smart_cast<CInventoryOwner*>(Level().CurrentViewEntity());
    relation = RELATION_REGISTRY().GetRelationType(pInvOwner, pActor);
    shared_str sname = RELATION_REGISTRY().GetSpotName(relation);

    CEntityAlive* pEntAlive = smart_cast<CEntityAlive*>(pInvOwner);
    if (!pEntAlive->g_Alive())
        sname = "deadbody_location";

    if (CMapLocation* location = GetMapLocation(sname, pInvOwner->object_id()); location)
    {
        const bool isCorrect = smart_cast<CRelationMapLocation*>(location);
#ifdef DEBUG
        Msg("~ CMapManager: Someone has already added relation location%s. [%s of %s]",
            (isCorrect ? "" : " as map location"), sname.c_str(), pInvOwner->Name());
#endif
        location->UpdateTTL();
        return location;
    }
    CMapLocation* l = xr_new<CRelationMapLocation>(sname, pInvOwner->object_id(), pActor->object_id());
    Locations().push_back(SLocationKey(sname, pInvOwner->object_id()));
    Locations().back().location = l;
    return l;
}

void CMapManager::Destroy(CMapLocation* ml) { m_deffered_destroy_queue.push_back(ml); }

void CMapManager::OnUIReset()
{
    for (const SLocationKey& locationKey : Locations())
    {
        locationKey.location->LoadSpot(locationKey.spot_type.c_str());
    }
}

void CMapManager::RemoveMapLocation(const shared_str& spot_type, u16 id)
{
    FindLocationBySpotID key(spot_type, id);
    auto it = std::find_if(Locations().begin(), Locations().end(), key);
    if (it != Locations().end())
    {
        if (IsGameTypeSingle())
            Level().GameTaskManager().MapLocationRelcase((*it).location);

        Destroy((*it).location);
        Locations().erase(it);
    }
}

void CMapManager::RemoveMapLocationByObjectID(u16 id) // call on destroy object
{
    FindLocationByID key(id);
    auto it = std::find_if(Locations().begin(), Locations().end(), key);
    while (it != Locations().end())
    {
        if (IsGameTypeSingle())
            Level().GameTaskManager().MapLocationRelcase((*it).location);

        Destroy((*it).location);
        Locations().erase(it);

        it = std::find_if(Locations().begin(), Locations().end(), key);
    }
}

void CMapManager::RemoveMapLocation(CMapLocation* ml)
{
    FindLocation key(ml);

    auto it = std::find_if(Locations().begin(), Locations().end(), key);
    if (it != Locations().end())
    {
        if (IsGameTypeSingle())
            Level().GameTaskManager().MapLocationRelcase((*it).location);

        Destroy((*it).location);
        Locations().erase(it);
    }
}

bool CMapManager::GetMapLocationsForObject(u16 id, xr_vector<CMapLocation*>& res)
{
    res.clear();
    auto it = Locations().begin();
    auto it_e = Locations().end();
    for (; it != it_e; ++it)
    {
        if ((*it).actual && (*it).object_id == id)
            res.push_back((*it).location);
    }
    return (res.size() != 0);
}

bool CMapManager::HasMapLocation(const shared_str& spot_type, u16 id)
{
    CMapLocation* l = GetMapLocation(spot_type, id);

    return (l != NULL);
}

CMapLocation* CMapManager::GetMapLocation(const shared_str& spot_type, u16 id)
{
    FindLocationBySpotID key(spot_type, id);
    auto it = std::find_if(Locations().begin(), Locations().end(), key);
    if (it != Locations().end())
        return (*it).location;

    return 0;
}

void CMapManager::GetMapLocations(const shared_str& spot_type, u16 id, xr_vector<CMapLocation*>& res)
{
    FindLocationBySpotID key(spot_type, id);
    auto it = std::find_if(Locations().begin(), Locations().end(), key);

    while (it != Locations().end())
    {
        res.push_back((*it).location);
        it = std::find_if(++it, Locations().end(), key);
    }
}

void CMapManager::Update()
{
    delete_data(m_deffered_destroy_queue); // from prev frame

    auto it = Locations().begin();
    auto it_e = Locations().end();

    for (u32 idx = 0; it != it_e; ++it, ++idx)
    {
        bool bForce = Device.dwFrame % 3 == idx % 3;
        (*it).actual = (*it).location->Update();

        if ((*it).actual && bForce)
            (*it).location->CalcPosition();
    }
    std::sort(Locations().begin(), Locations().end());

    while ((!Locations().empty()) && (!Locations().back().actual))
    {
        if (IsGameTypeSingle())
            Level().GameTaskManager().MapLocationRelcase(Locations().back().location);

        Destroy(Locations().back().location);
        Locations().pop_back();
    }
}

void CMapManager::DisableAllPointers()
{
    auto it = Locations().begin();
    auto it_e = Locations().end();

    for (; it != it_e; ++it)
        (*it).location->DisablePointer();
}

vLocations& CMapManager::Locations()
{
    if (!m_locations)
    {
        m_locations = &m_locations_wrapper->registry().objects();
#ifdef DEBUG
        Msg("m_locations size=%d", m_locations->size());
#endif // #ifdef DEBUG
    }
    return *m_locations;
}

void CMapManager::OnObjectDestroyNotify(u16 id) { RemoveMapLocationByObjectID(id); }
#ifdef DEBUG
void CMapManager::Dump()
{
    Msg("begin of map_locations dump");
    auto it = Locations().begin();
    auto it_e = Locations().end();
    for (; it != it_e; ++it)
    {
        Msg("spot_type=[%s] object_id=[%d]", *((*it).spot_type), (*it).object_id);
        (*it).location->Dump();
    }

    Msg("end of map_locations dump");
}
#endif
