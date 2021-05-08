////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Object_Base.h
//  Created     : 19.09.2002
//  Modified    : 16.07.2004
//  Author      : Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//  Description : Server base object
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrServer_Objects_Abstract.h"
#include "Common/object_interfaces.h"
#include "script_value_container.h"
#include "alife_space.h"
#include "xrCore/client_id.h"
#include "xrCore/clsid.h"

class NET_Packet;
class xrClientData;
class CSE_ALifeGroupAbstract;
class CSE_ALifeSchedulable;
class CSE_ALifeInventoryItem;
class CSE_ALifeTraderAbstract;
class CSE_ALifeObject;
class CSE_ALifeDynamicObject;
class CSE_ALifeItemAmmo;
class CSE_ALifeItemWeapon;
class CSE_ALifeItemDetector;
class CSE_ALifeMonsterAbstract;
class CSE_ALifeHumanAbstract;
class CSE_ALifeAnomalousZone;
class CSE_ALifeTrader;
class CSE_ALifeCreatureAbstract;
class CSE_ALifeSmartZone;
class CSE_ALifeOnlineOfflineGroup;
class CSE_ALifeItemPDA;

#pragma warning(push)
#pragma warning(disable : 4005)

class CPureServerObject : public IPureServerObject
{
    using inherited = IPureServerObject;

public:
    virtual ~CPureServerObject() {}
    virtual void load(IReader& tFileStream);
    virtual void save(IWriter& tMemoryStream);
    virtual void load(NET_Packet& tNetPacket);
    virtual void save(NET_Packet& tNetPacket);
};

class CSE_Abstract : public IServerEntity, public CPureServerObject, public CScriptValueContainer
{
    using inherited1 = IServerEntity;
    using inherited2 = CPureServerObject;
    using inherited3 = CScriptValueContainer;

public:
    enum ESpawnFlags
    {
        flSpawnEnabled = u32(1 << 0),
        flSpawnOnSurgeOnly = u32(1 << 1),
        flSpawnSingleItemOnly = u32(1 << 2),
        flSpawnIfDestroyedOnly = u32(1 << 3),
        flSpawnInfiniteCount = u32(1 << 4),
        flSpawnDestroyOnSpawn = u32(1 << 5),
    };

private:
    pstr s_name_replace;

public:
    BOOL net_Ready;
    BOOL net_Processed; // Internal flag for connectivity-graph

    u16 m_wVersion;
    u16 m_script_version;
    u16 RespawnTime;

    u16 ID; // internal ID
    u16 ID_Parent; // internal ParentID, 0xffff means no parent
    u16 ID_Phantom; // internal PhantomID, 0xffff means no phantom
    xrClientData* owner;

    // spawn data
    shared_str s_name;
    //. u8                              s_gameid;
    GameTypeChooser m_gameType;
    u8 s_RP;
    Flags16 s_flags; // state flags
    xr_vector<u16> children;

    // update data
    Fvector o_Position;
    Fvector o_Angle;
    CLASS_ID m_tClassID;
    int m_script_clsid;
    shared_str m_ini_string;
    CInifile* m_ini_file;

    // for ALife control
    bool m_bALifeControl;
    ALife::_SPAWN_ID m_tSpawnID;

    // ALife spawn params
    // obsolete, just because we hope to uncomment all this stuff
    Flags32 m_spawn_flags;

    // client object custom data serialization
    xr_vector<u8> client_data;
    virtual void load(NET_Packet& tNetPacket);

    //////////////////////////////////////////////////////////////////////////

    CSE_Abstract(LPCSTR caSection);
    virtual ~CSE_Abstract();
    virtual void OnEvent(NET_Packet& /*tNetPacket*/, u16 /*type*/, u32 /*time*/, ClientID /*sender*/){}
#ifndef XRGAME_EXPORTS
    virtual void FillProps(LPCSTR pref, PropItemVec& items);
    virtual void __stdcall FillProp(LPCSTR pref, PropItemVec& items);
    virtual void __stdcall on_render(CDUInterface* /*du*/, IServerEntityLEOwner* /*owner*/, bool /*bSelected*/,
        const Fmatrix& /*parent*/, int /*priority*/, bool /*strictB2F*/)
    {
    }
    virtual visual_data* __stdcall visual_collection() const { return nullptr; }
    virtual u32 __stdcall visual_collection_size() const { return 0; }
    virtual void __stdcall set_additional_info(void* /*info*/) {};
#endif // #ifndef XRGAME_EXPORTS
    virtual BOOL Net_Relevant() { return FALSE; } // !!!! WARNING!!!
    //
    virtual void __stdcall Spawn_Write(NET_Packet& tNetPacket, BOOL bLocal);
    virtual BOOL __stdcall Spawn_Read(NET_Packet& tNetPacket);
    virtual LPCSTR __stdcall name() const;
    virtual LPCSTR __stdcall name_replace() const;
    virtual void __stdcall set_name(LPCSTR s) { s_name = s; }
    virtual void __stdcall set_name_replace(LPCSTR s)
    {
        xr_free(s_name_replace);
        s_name_replace = xr_strdup(s);
    };
    virtual Fvector& __stdcall position();
    virtual Fvector& __stdcall angle();
    virtual Flags16& __stdcall flags();
    virtual CSE_Visual* __stdcall visual();
    virtual IServerEntityShape* __stdcall shape();
    virtual CSE_Motion* __stdcall motion();
    virtual bool __stdcall validate();
    //

    IC const Fvector& Position() const { return o_Position; }
    // we need this to prevent virtual inheritance :-(
    virtual CSE_Abstract* base();
    virtual const CSE_Abstract* base() const;
    virtual CSE_Abstract* init();
    virtual bool match_configuration() const /* noexcept */ { return true; }
    // end of the virtual inheritance dependant code
    IC int script_clsid() const
    {
        VERIFY(m_script_clsid >= 0);
        return (m_script_clsid);
    }
    CInifile& spawn_ini();

    // for smart cast
    virtual CSE_ALifeGroupAbstract* cast_group_abstract() { return nullptr; }
    virtual CSE_ALifeSchedulable* cast_schedulable() { return nullptr; }
    virtual CSE_ALifeInventoryItem* cast_inventory_item() { return nullptr; }
    virtual CSE_ALifeTraderAbstract* cast_trader_abstract() { return nullptr; }
    virtual CSE_ALifeObject* cast_alife_object() { return nullptr; }
    virtual CSE_ALifeDynamicObject* cast_alife_dynamic_object() { return nullptr; }
    virtual CSE_ALifeItemAmmo* cast_item_ammo() { return nullptr; }
    virtual CSE_ALifeItemWeapon* cast_item_weapon() { return nullptr; }
    virtual CSE_ALifeItemDetector* cast_item_detector() { return nullptr; }
    virtual CSE_ALifeMonsterAbstract* cast_monster_abstract() { return nullptr; }
    virtual CSE_ALifeHumanAbstract* cast_human_abstract() { return nullptr; }
    virtual CSE_ALifeAnomalousZone* cast_anomalous_zone() { return nullptr; }
    virtual CSE_ALifeTrader* cast_trader() { return nullptr; }
    virtual CSE_ALifeCreatureAbstract* cast_creature_abstract() { return nullptr; }
    virtual CSE_ALifeSmartZone* cast_smart_zone() { return nullptr; }
    virtual CSE_ALifeOnlineOfflineGroup* cast_online_offline_group() { return nullptr; }
    virtual CSE_ALifeItemPDA* cast_item_pda() { return nullptr; }
};

extern u16 script_server_object_version();

#pragma warning(pop)
