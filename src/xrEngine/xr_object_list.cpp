#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "xrSheduler.h"
#include "xr_object_list.h"
#include "std_classes.h"

#include "xr_object.h"
#include "xrCore/net_utils.h"

#include "CustomHUD.h"
#include "GameFont.h"
#include "PerformanceAlert.hpp"

class fClassEQ
{
    CLASS_ID cls;

public:
    fClassEQ(CLASS_ID C) : cls(C){};
    IC bool operator()(IGameObject* O) { return cls == O->GetClassId(); }
};
#ifdef DEBUG
ENGINE_API BOOL debug_destroy = TRUE;
#endif

void CObjectList::DumpStatistics(IGameFont& font, IPerformanceAlert* alert)
{
    stats.FrameEnd();
    float engineTotal = Device.GetStats().EngineTotal.result;
    float percentage = 100.0f * stats.Update.result / engineTotal;
    font.OutNext("Objects:      %2.2fms, %2.1f%%", stats.Update.result, percentage);
    font.OutNext("- crow:       %d", stats.Crows);
    font.OutNext("- active:     %d", stats.Active);
    font.OutNext("- total:      %d", stats.Total);
    if (alert && stats.Update.result > 3.0f)
        alert->Print(font, "UpdateCL  > 3ms:  %3.1f", stats.Update.result);
}

CObjectList::CObjectList() : m_owner_thread_id(GetCurrentThreadId())
{
    statsFrame = u32(-1);
    ZeroMemory(map_NETID, 0xffff * sizeof(IGameObject*));
}

CObjectList::~CObjectList()
{
    R_ASSERT(objects_active.empty());
    R_ASSERT(objects_sleeping.empty());
    R_ASSERT(destroy_queue.empty());
    //. R_ASSERT ( map_NETID.empty() );
}

IGameObject* CObjectList::FindObjectByName(shared_str name)
{
    for (auto& it : objects_active)
        if (it->cName().equal(name))
            return it;

    for (auto& it : objects_sleeping)
        if (it->cName().equal(name))
            return it;

    return nullptr;
}
IGameObject* CObjectList::FindObjectByName(LPCSTR name) { return FindObjectByName(shared_str(name)); }
IGameObject* CObjectList::FindObjectByCLS_ID(CLASS_ID cls)
{
    {
        Objects::iterator O = std::find_if(objects_active.begin(), objects_active.end(), fClassEQ(cls));
        if (O != objects_active.end())
            return *O;
    }
    {
        Objects::iterator O = std::find_if(objects_sleeping.begin(), objects_sleeping.end(), fClassEQ(cls));
        if (O != objects_sleeping.end())
            return *O;
    }

    return NULL;
}

void CObjectList::o_remove(Objects& v, IGameObject* O)
{
    //. if(O->ID()==1026)
    //. {
    //. Log("ahtung");
    //. }
    Objects::iterator _i = std::find(v.begin(), v.end(), O);
    VERIFY(_i != v.end());
    v.erase(_i);
    //. Msg("---o_remove[%s][%d]", O->cName().c_str(), O->ID() );
}

void CObjectList::o_activate(IGameObject* O)
{
    VERIFY(O && O->processing_enabled());
    o_remove(objects_sleeping, O);
    objects_active.push_back(O);
    O->MakeMeCrow();
}
void CObjectList::o_sleep(IGameObject* O)
{
    VERIFY(O && !O->processing_enabled());
    o_remove(objects_active, O);
    objects_sleeping.push_back(O);
    O->MakeMeCrow();
}

void CObjectList::SingleUpdate(IGameObject* O)
{
    if (Device.dwFrame == O->GetUpdateFrame())
    {
#ifdef DEBUG
// if (O->getDestroy())
// Msg ("- !!!processing_enabled ->destroy_queue.push_back %s[%d] frame [%d]",O->cName().c_str(), O->ID(),
// Device.dwFrame);
#endif // #ifdef DEBUG

        return;
    }

    if (!O->processing_enabled())
    {
#ifdef DEBUG
// if (O->getDestroy())
// Msg ("- !!!processing_enabled ->destroy_queue.push_back %s[%d] frame [%d]",O->cName().c_str(), O->ID(),
// Device.dwFrame);
#endif // #ifdef DEBUG

        return;
    }

    if (O->H_Parent())
        SingleUpdate(O->H_Parent());
    stats.Updated++;
    O->SetUpdateFrame(Device.dwFrame);

    // Msg ("[%d][0x%08x]IAmNotACrowAnyMore (CObjectList::SingleUpdate)", Device.dwFrame, dynamic_cast<void*>(O));

    O->UpdateCL();

    VERIFY3(O->GetDbgUpdateFrame() == Device.dwFrame, "Broken sequence of calls to 'UpdateCL'", *O->cName());
#if 0 // ndef DEBUG
    __try
    {
#endif
    if (O->H_Parent() && (O->H_Parent()->getDestroy() || O->H_Root()->getDestroy()))
    {
        // Push to destroy-queue if it isn't here already
        Msg("! ERROR: incorrect destroy sequence for object[%d:%s], section[%s], parent[%d:%s]", O->ID(), *O->cName(),
            *O->cNameSect(), O->H_Parent()->ID(), *O->H_Parent()->cName());
    }
#if 0 // ndef DEBUG
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        IGameObject* parent_obj = O->H_Parent();
        IGameObject* root_obj = O->H_Root();
        Msg ("! ERROR: going to crush: [%d:%s], section[%s], parent_obj_addr[0x%08x], root_obj_addr[0x%08x]",O->ID(),*O->cName(),*O->cNameSect(), *((u32*)&parent_obj), *((u32*)&root_obj));
        if (parent_obj)
        {
            __try
            {
                Msg("! Parent object: [%d:%s], section[%s]",
                    parent_obj->ID(),
                    parent_obj->cName().c_str(),
                    parent_obj->cNameSect().c_str());

            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Msg("! Failed to get parent object info.");
            }
        }
        if (root_obj)
        {
            __try
            {
                Msg("! Root object: [%d:%s], section[%s]",
                    root_obj->ID(),
                    root_obj->cName().c_str(),
                    root_obj->cNameSect().c_str());
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                Msg("! Failed to get root object info.");
            }
        }
        R_ASSERT(false);
    } //end of __except
#endif

#ifdef DEBUG
// if (O->getDestroy())
// Msg ("- !!!processing_enabled ->destroy_queue.push_back %s[%d] frame [%d]",O->cName().c_str(), O->ID(),
// Device.dwFrame);
#endif // #ifdef DEBUG
}

void CObjectList::clear_crow_vec(Objects& o)
{
    for (auto& it : o)
    {
        // Msg ("[%d][0x%08x]IAmNotACrowAnyMore (clear_crow_vec)", Device.dwFrame, dynamic_cast<void*>(it));
        it->IAmNotACrowAnyMore();
    }
    o.clear();
}

void CObjectList::Update(bool bForce)
{
    if (statsFrame != Device.dwFrame)
    {
        statsFrame = Device.dwFrame;
        stats.FrameStart();
    }
    if (!Device.Paused() || bForce)
    {
        // Clients
        if (Device.fTimeDelta > EPS_S || bForce)
        {
            // Select Crow-Mode
            stats.Updated = 0;

            m_primary_crows.insert(m_primary_crows.end(), m_secondary_crows.begin(), m_secondary_crows.end());
            m_secondary_crows.clear();

#if 0
            std::sort (m_own_crows.begin(), m_own_crows.end());
            m_own_crows.erase (
                std::unique(
                    m_own_crows.begin(),
                    m_own_crows.end()
                ),
                m_own_crows.end()
            );
#else
#ifdef DEBUG
            std::sort(m_primary_crows.begin(), m_primary_crows.end());
            VERIFY(std::unique(m_primary_crows.begin(), m_primary_crows.end()) == m_primary_crows.end());
#endif // ifdef DEBUG
#endif

            stats.Crows = m_primary_crows.size();
            Objects* workload;
            if (!psDeviceFlags.test(rsDisableObjectsAsCrows))
                workload = &m_primary_crows;
            else
            {
                workload = &objects_active;
                clear_crow_vec(m_primary_crows);
            }

            stats.Update.Begin();
            stats.Active = objects_active.size();
            stats.Total = objects_active.size() + objects_sleeping.size();

            u32 const objects_count = workload->size();
            IGameObject** objects = (IGameObject**)_alloca(objects_count * sizeof(IGameObject*));
            std::copy(workload->begin(), workload->end(), objects);

            m_primary_crows.clear();

            IGameObject** b = objects;
            IGameObject** e = objects + objects_count;
            for (IGameObject** i = b; i != e; ++i)
            {
                (*i)->IAmNotACrowAnyMore();
                (*i)->SetCrowUpdateFrame(u32(-1));
            }

            for (IGameObject** i = b; i != e; ++i)
                SingleUpdate(*i);

            //--#SM+#-- PostUpdateCL для всех клиентских объектов [for crowed and non-crowed]
            for (auto& object : objects_active)
                object->PostUpdateCL(false);

            for (auto& object : objects_sleeping)
                object->PostUpdateCL(true);

            stats.Update.End();
        }
    }

    // Destroy
    if (!destroy_queue.empty())
    {
        // Info
        for (Objects::iterator oit = objects_active.begin(); oit != objects_active.end(); oit++)
            for (int it = destroy_queue.size() - 1; it >= 0; it--)
            {
                (*oit)->net_Relcase(destroy_queue[it]);
            }
        for (Objects::iterator oit = objects_sleeping.begin(); oit != objects_sleeping.end(); oit++)
            for (int it = destroy_queue.size() - 1; it >= 0; it--)
                (*oit)->net_Relcase(destroy_queue[it]);

        for (int it = destroy_queue.size() - 1; it >= 0; it--)
            GEnv.Sound->object_relcase(destroy_queue[it]);

        RELCASE_CALLBACK_VEC::iterator it = m_relcase_callbacks.begin();
        const RELCASE_CALLBACK_VEC::iterator ite = m_relcase_callbacks.end();
        for (; it != ite; ++it)
        {
            VERIFY(*(*it).m_ID == (it - m_relcase_callbacks.begin()));
            for (auto& dit : destroy_queue)
            {
                (*it).m_Callback(dit);
                g_hud->net_Relcase(dit);
            }
        }

        // Destroy
        for (int it = destroy_queue.size() - 1; it >= 0; it--)
        {
            IGameObject* O = destroy_queue[it];
// Msg ("Object [%x]", O);
#ifdef DEBUG
            if (debug_destroy)
                Msg("Destroying object[%x][%x] [%d][%s] frame[%d]", dynamic_cast<void*>(O), O, O->ID(), *O->cName(),
                    Device.dwFrame);
#endif // DEBUG
            O->net_Destroy();
            Destroy(O);
        }
        destroy_queue.clear();
    }
}

void CObjectList::net_Register(IGameObject* O)
{
    R_ASSERT(O);
    R_ASSERT(O->ID() < 0xffff);

    map_NETID[O->ID()] = O;
    //. map_NETID.insert(std::make_pair(O->ID(),O));
    // Msg ("-------------------------------- Register: %s",O->cName());
}

void CObjectList::net_Unregister(IGameObject* O)
{
    // R_ASSERT (O->ID() < 0xffff);
    if (O->ID() < 0xffff) // demo_spectator can have 0xffff
        map_NETID[O->ID()] = NULL;
    /*
     xr_map<u32,IGameObject*>::iterator it = map_NETID.find(O->ID());
     if ((it!=map_NETID.end()) && (it->second == O)) {
     // Msg ("-------------------------------- Unregster: %s",O->cName());
     map_NETID.erase(it);
     }
     */
}

int g_Dump_Export_Obj = 0;

u32 CObjectList::net_Export(NET_Packet* _Packet, u32 start, u32 max_object_size)
{
    if (g_Dump_Export_Obj)
        Msg("---- net_export --- ");

    NET_Packet& Packet = *_Packet;
    u32 position;
    for (; start < objects_active.size() + objects_sleeping.size(); start++)
    {
        IGameObject* P =
            (start < objects_active.size()) ? objects_active[start] : objects_sleeping[start - objects_active.size()];
        if (P->net_Relevant() && !P->getDestroy())
        {
            Packet.w_u16(u16(P->ID()));
            Packet.w_chunk_open8(position);
            // Msg ("cl_export: %d '%s'",P->ID(),*P->cName());
            P->net_Export(Packet);

#ifdef DEBUG
            {
                u32 size = u32(Packet.w_tell() - position) - sizeof(u8);
                if (size >= 256)
                {
                    xrDebug::Fatal(DEBUG_INFO, "Object [%s][%d] exceed network-data limit\n size=%d, Pend=%d, Pstart=%d",
                    *P->cName(), P->ID(), size, Packet.w_tell(), position);
                }
            }
#endif
            if (g_Dump_Export_Obj)
            {
                u32 size = u32(Packet.w_tell() - position) - sizeof(u8);
                Msg("* %s : %d", *(P->cNameSect()), size);
            }
            Packet.w_chunk_close8(position);
            // if (0==(--count))
            // break;
            if (max_object_size >= (NET_PacketSizeLimit - Packet.w_tell()))
                break;
        }
    }
    if (g_Dump_Export_Obj)
        Msg("------------------- ");
    return start + 1;
}

int g_Dump_Import_Obj = 0;

void CObjectList::net_Import(NET_Packet* Packet)
{
    if (g_Dump_Import_Obj)
        Msg("---- net_import --- ");

    while (!Packet->r_eof())
    {
        u16 ID;
        Packet->r_u16(ID);
        u8 size;
        Packet->r_u8(size);
        IGameObject* P = net_Find(ID);
        if (P)
        {
            u32 rsize = Packet->r_tell();

            P->net_Import(*Packet);

            if (g_Dump_Import_Obj)
                Msg("* %s : %d - %d", *(P->cNameSect()), size, Packet->r_tell() - rsize);
        }
        else
            Packet->r_advance(size);
    }

    if (g_Dump_Import_Obj)
        Msg("------------------- ");
}

/*
IGameObject* CObjectList::net_Find(u16 ID)
{

xr_map<u32,IGameObject*>::iterator it = map_NETID.find(ID);
return (it==map_NETID.end())?0:it->second;
}
*/
void CObjectList::Load()
{
    R_ASSERT(/*map_NETID.empty() &&*/ objects_active.empty() && destroy_queue.empty() && objects_sleeping.empty());
}

void CObjectList::Unload()
{
    if (objects_sleeping.size() || objects_active.size())
        Msg("! objects-leaked: %d", objects_sleeping.size() + objects_active.size());

    // Destroy objects
    while (objects_sleeping.size())
    {
        IGameObject* O = objects_sleeping.back();
        Msg("! [%x] s[%4d]-[%s]-[%s]", O, O->ID(), *O->cNameSect(), *O->cName());
        O->setDestroy(true);

#ifdef DEBUG
        if (debug_destroy)
            Msg("Destroying object [%d][%s]", O->ID(), *O->cName());
#endif
        O->net_Destroy();
        Destroy(O);
    }
    while (objects_active.size())
    {
        IGameObject* O = objects_active.back();
        Msg("! [%x] a[%4d]-[%s]-[%s]", O, O->ID(), *O->cNameSect(), *O->cName());
        O->setDestroy(true);

#ifdef DEBUG
        if (debug_destroy)
            Msg("Destroying object [%d][%s]", O->ID(), *O->cName());
#endif
        O->net_Destroy();
        Destroy(O);
    }
}

IGameObject* CObjectList::Create(LPCSTR name)
{
    IGameObject* O = g_pGamePersistent->ObjectPool.create(name);
    // Msg("CObjectList::Create [%x]%s", O, name);
    objects_sleeping.push_back(O);
    return O;
}

void CObjectList::Destroy(IGameObject* game_obj)
{
    if (nullptr == game_obj)
        return;
    net_Unregister(game_obj);

    if (!Device.Paused())
    {
        // if a game is paused list of other crows should be empty - Why?
        if (!m_secondary_crows.empty())
        {
            Msg("assertion !m_other_crows.empty() failed: %d", m_secondary_crows.size());

            u32 j = 0;
            for (auto& iter : m_secondary_crows)
                Msg("%d %s", j++, iter->cName().c_str());
            VERIFY(Device.Paused() || m_secondary_crows.empty());
            m_secondary_crows.clear();
        }
    }
    else
    {
        // if game is paused remove the object from list of other crows
        auto iter = std::find(m_secondary_crows.begin(), m_secondary_crows.end(), game_obj);
        if (iter != m_secondary_crows.end())
            m_secondary_crows.erase(iter);
    }

    {
        // Always remove the object from list of own crows. The object may be not a crow.
        auto iter = std::find(m_primary_crows.begin(), m_primary_crows.end(), game_obj);
        if (iter != m_primary_crows.end())
            m_primary_crows.erase(iter);
    }

    // Remove the object from list of active objects if the object is active,
    // either remove it from list of sleeping objects if it is sleeping
    // and throw fatal error if the object is neither active nor sleeping
    auto iter = std::find(objects_active.begin(), objects_active.end(), game_obj);
    if (iter != objects_active.end())
    {
        // this object is active, so remove it from the list
        objects_active.erase(iter);
        // check that the object does not belong to list of sleeping object too
        VERIFY(std::find(objects_sleeping.begin(), objects_sleeping.end(), game_obj) == objects_sleeping.end());
    }
    else
    {
        iter = std::find(objects_sleeping.begin(), objects_sleeping.end(), game_obj);
        if (iter == objects_sleeping.end())
            FATAL("! Unregistered object being destroyed");
        objects_sleeping.erase(iter);

    }

    g_pGamePersistent->ObjectPool.destroy(game_obj);
}

void CObjectList::relcase_register(RELCASE_CALLBACK cb, int* ID)
{
#ifdef DEBUG
    RELCASE_CALLBACK_VEC::iterator It = std::find(m_relcase_callbacks.begin(), m_relcase_callbacks.end(), cb);
    VERIFY(It == m_relcase_callbacks.end());
#endif
    *ID = m_relcase_callbacks.size();
    m_relcase_callbacks.push_back(SRelcasePair(ID, cb));
}

void CObjectList::relcase_unregister(int* ID)
{
    VERIFY(m_relcase_callbacks[*ID].m_ID == ID);
    m_relcase_callbacks[*ID] = m_relcase_callbacks.back();
    *m_relcase_callbacks.back().m_ID = *ID;
    m_relcase_callbacks.pop_back();
}

void CObjectList::dump_list(Objects& v, LPCSTR reason)
{
#ifdef DEBUG
    for (auto& it : v)
        Msg("%x - name [%s] ID[%d] parent[%s] getDestroy()=[%s]", it, it->cName().c_str(), it->ID(),
        it->H_Parent() ? it->H_Parent()->cName().c_str() : "", it->getDestroy() ? "yes" : "no");
#endif // #ifdef DEBUG
}

bool CObjectList::dump_all_objects()
{
    dump_list(destroy_queue, "destroy_queue");
    dump_list(objects_active, "objects_active");
    dump_list(objects_sleeping, "objects_sleeping");
    dump_list(m_primary_crows, "m_own_crows");
    dump_list(m_secondary_crows, "m_other_crows");
    return false;
}

void CObjectList::register_object_to_destroy(IGameObject* object_to_destroy)
{
    VERIFY(!registered_object_to_destroy(object_to_destroy));
    // Msg("CObjectList::register_object_to_destroy [%x]", object_to_destroy);
    destroy_queue.push_back(object_to_destroy);

    for (auto& it : objects_active)
    {
        IGameObject* O = it;
        if (!O->getDestroy() && O->H_Parent() == object_to_destroy)
        {
            Msg("setDestroy called, but not-destroyed child found parent[%d] child[%d]", object_to_destroy->ID(),
                O->ID(), Device.dwFrame);
            O->setDestroy(true);
        }
    }

    for (auto& it : objects_sleeping)
    {
        IGameObject* O = it;
        if (!O->getDestroy() && O->H_Parent() == object_to_destroy)
        {
            Msg("setDestroy called, but not-destroyed child found parent[%d] child[%d]", object_to_destroy->ID(),
                O->ID(), Device.dwFrame);
            O->setDestroy(true);
        }
    }
}

#ifdef DEBUG
bool CObjectList::registered_object_to_destroy(const IGameObject* object_to_destroy) const
{
    return (std::find(destroy_queue.begin(), destroy_queue.end(), object_to_destroy) != destroy_queue.end());
}
#endif // DEBUG
