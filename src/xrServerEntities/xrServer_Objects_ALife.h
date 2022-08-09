////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_ALife.h
//  Created     : 19.09.2002
//  Modified    : 04.06.2003
//  Author      : Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//  Description : Server objects for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALifeH
#define xrServer_Objects_ALifeH
//#include "pch_script.h" - No, no NO!
#include "xrServer_Objects.h"
#include "alife_space.h"
#include "xrAICore/Navigation/game_graph_space.h"
#ifdef XRGAME_EXPORTS
#include "xrGame/alife_smart_terrain_task.h"
#endif //#ifdef XRGAME_EXPORTS

#pragma warning(push)
#pragma warning(disable : 4005)

#ifdef XRGAME_EXPORTS
class CALifeSimulator;
#endif

class CSE_ALifeItemWeapon;
class CSE_ALifeDynamicObject;
class CSE_ALifeObject;
#ifdef XRGAME_EXPORTS
class CALifeSmartTerrainTask;
#endif //#ifdef XRGAME_EXPORTS
class CALifeMonsterAbstract;
class CSE_ALifeInventoryItem;

struct SFillPropData
{
    RTokenVec locations[4];
    RStringVec level_ids;
    RTokenVec story_names;
    RTokenVec spawn_story_names;
    RStringVec character_profiles;
    RStringVec smart_covers;
    xr_map<shared_str, u32> location_colors;
    u32 counter;
    SFillPropData();
    ~SFillPropData();
    void load();
    void unload();
    void inc();
    void dec();
};

class CSE_ALifeSchedulable : public IPureSchedulableObject
{
    using inherited = IPureSchedulableObject;

public:
    CSE_ALifeItemWeapon* m_tpCurrentBestWeapon;
    CSE_ALifeDynamicObject* m_tpBestDetector;
    u64 m_schedule_counter;

    CSE_ALifeSchedulable(LPCSTR caSection);
    virtual ~CSE_ALifeSchedulable();
    // we need this to prevent virtual inheritance :-(
    virtual CSE_Abstract* base() = 0;
    virtual const CSE_Abstract* base() const = 0;
    virtual CSE_Abstract* init();
    virtual CSE_ALifeSchedulable* cast_schedulable() { return this; };
    virtual CSE_Abstract* cast_abstract() { return nullptr; };
    // end of the virtual inheritance dependant code
    virtual bool need_update(CSE_ALifeDynamicObject* object);
    virtual u32 ef_creature_type() const;
    virtual u32 ef_anomaly_type() const;
    virtual u32 ef_weapon_type() const;
    virtual u32 ef_detector_type() const;
    virtual bool natural_weapon() const { return true; }
    virtual bool natural_detector() const { return true; }
#ifdef XRGAME_EXPORTS
    virtual CSE_ALifeItemWeapon* tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower) = 0;
    virtual bool bfPerformAttack() { return (true); };
    virtual void vfUpdateWeaponAmmo(){};
    virtual void vfProcessItems(){};
    virtual void vfAttachItems(ALife::ETakeType tTakeType = ALife::eTakeTypeAll){};
    virtual ALife::EMeetActionType tfGetActionType(
        CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex, bool bMutualDetection) = 0;
    virtual bool bfActive() = 0;
    virtual CSE_ALifeDynamicObject* tpfGetBestDetector() = 0;
#endif
};

class CSE_ALifeGraphPoint : public CSE_Abstract
{
    using inherited = CSE_Abstract;

public:
    shared_str m_caConnectionLevelName;
    shared_str m_caConnectionPointName;
    u8 m_tLocations[GameGraph::LOCATION_TYPE_COUNT];

    CSE_ALifeGraphPoint(LPCSTR caSection);
    virtual ~CSE_ALifeGraphPoint();
    virtual bool match_configuration() const /* noexcept */ { return false; }
#ifndef XRGAME_EXPORTS
    virtual void __stdcall on_render(CDUInterface* du, IServerEntityLEOwner* owner, bool bSelected,
        const Fmatrix& parent, int priority, bool strictB2F);
#endif
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeObject : public CSE_Abstract, public CRandom
{
    using inherited1 = CSE_Abstract;
    using inherited2 = CRandom;

public:
    enum
    {
        flUseSwitches = u32(1) << 0,
        flSwitchOnline = u32(1) << 1,
        flSwitchOffline = u32(1) << 2,
        flInteractive = u32(1) << 3,
        flVisibleForAI = u32(1) << 4,
        flUsefulForAI = u32(1) << 5,
        flOfflineNoMove = u32(1) << 6,
        flUsedAI_Locations = u32(1) << 7,
        flGroupBehaviour = u32(1) << 8,
        flCanSave = u32(1) << 9,
        flVisibleForMap = u32(1) << 10,
        flUseSmartTerrains = u32(1) << 11,
        flCheckForSeparator = u32(1) << 12,
    };

public:
    using inherited = CSE_Abstract;
    GameGraph::_GRAPH_ID m_tGraphID;
    float m_fDistance;
    bool m_bOnline;
    bool m_bDirectControl;
    u32 m_tNodeID;
    flags32 m_flags;
    ALife::_STORY_ID m_story_id;
    ALife::_SPAWN_STORY_ID m_spawn_story_id;

#ifdef XRGAME_EXPORTS
    CALifeSimulator* m_alife_simulator;
#endif

    CSE_ALifeObject(LPCSTR caSection);
    virtual ~CSE_ALifeObject();
    virtual bool used_ai_locations() const /* noexcept* */;
    virtual bool can_save() const /* noexcept */;
    virtual bool can_switch_online() const /* noexcept */;
    virtual bool can_switch_offline() const /* noexcept */;
    virtual bool interactive() const /* noexcept */;
    virtual CSE_ALifeObject* cast_alife_object() { return this; }
    bool move_offline() const;
    void can_switch_online(bool value) /* noexcept */;
    void can_switch_offline(bool value) /* noexcept */;
    void use_ai_locations(bool value);
    void interactive(bool value) /* noexcept */;
    void move_offline(bool value);
    bool visible_for_map() const;
    void visible_for_map(bool value);
    virtual u32 ef_equipment_type() const;
    virtual u32 ef_main_weapon_type() const;
    virtual u32 ef_weapon_type() const;
    virtual u32 ef_detector_type() const;
#ifdef XRGAME_EXPORTS
    virtual void spawn_supplies(LPCSTR);
    virtual void spawn_supplies();
    CALifeSimulator& alife() const;
    virtual Fvector draw_level_position() const;
    virtual bool keep_saved_data_anyway() const /* noexcept */;
#endif
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeGroupAbstract
{
public:
    ALife::OBJECT_VECTOR m_tpMembers;
    bool m_bCreateSpawnPositions;
    u16 m_wCount;
    ALife::_TIME_ID m_tNextBirthTime;

    CSE_ALifeGroupAbstract(LPCSTR caSection);
    virtual ~CSE_ALifeGroupAbstract();
    virtual CSE_Abstract* init();
    virtual CSE_Abstract* base() = 0;
    virtual const CSE_Abstract* base() const = 0;
    virtual CSE_ALifeGroupAbstract* cast_group_abstract() { return this; };
    virtual CSE_Abstract* cast_abstract() { return nullptr; };
#ifdef XRGAME_EXPORTS
    virtual bool synchronize_location();
    virtual void try_switch_online();
    virtual void try_switch_offline();
    virtual void switch_online();
    virtual void switch_offline();
    virtual bool redundant() const;
#endif
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

template <class __A>
class CSE_ALifeGroupTemplate : public __A, public CSE_ALifeGroupAbstract
{
    using inherited1 = __A;
    using inherited2 = CSE_ALifeGroupAbstract;

public:
    CSE_ALifeGroupTemplate(LPCSTR caSection)
        : __A(pSettings->line_exist(caSection, "monster_section") ? pSettings->r_string(caSection, "monster_section") :
                                                                    caSection),
          CSE_ALifeGroupAbstract(caSection){};

    virtual ~CSE_ALifeGroupTemplate(){};

    virtual void STATE_Read(NET_Packet& tNetPacket, u16 size)
    {
        inherited1::STATE_Read(tNetPacket, size);
        inherited2::STATE_Read(tNetPacket, size);
    };

    virtual void STATE_Write(NET_Packet& tNetPacket)
    {
        inherited1::STATE_Write(tNetPacket);
        inherited2::STATE_Write(tNetPacket);
    };

    virtual void UPDATE_Read(NET_Packet& tNetPacket)
    {
        inherited1::UPDATE_Read(tNetPacket);
        inherited2::UPDATE_Read(tNetPacket);
    };

    virtual void UPDATE_Write(NET_Packet& tNetPacket)
    {
        inherited1::UPDATE_Write(tNetPacket);
        inherited2::UPDATE_Write(tNetPacket);
    };

    virtual CSE_Abstract* init()
    {
        inherited1::init();
        inherited2::init();
        return (base());
    }

    virtual CSE_Abstract* base() { return (inherited1::base()); }
    virtual const CSE_Abstract* base() const { return (inherited1::base()); }
#ifndef XRGAME_EXPORTS
    virtual void FillProps(LPCSTR pref, PropItemVec& items)
    {
        inherited1::FillProps(pref, items);
        inherited2::FillProps(pref, items);
    };
#endif // #ifndef XRGAME_EXPORTS

    virtual CSE_Abstract* cast_abstract() { return (this); }
    virtual CSE_ALifeGroupAbstract* cast_group_abstract() { return (this); }
#ifdef XRGAME_EXPORTS
    virtual void switch_online() { inherited2::switch_online(); }
    virtual void switch_offline() { inherited2::switch_offline(); }
    virtual bool synchronize_location() { return (inherited2::synchronize_location()); }
    virtual void try_switch_online() { inherited2::try_switch_online(); }
    virtual void try_switch_offline() { inherited2::try_switch_offline(); }
    virtual bool redundant() const { return (inherited2::redundant()); }
#endif
};

class CSE_ALifeDynamicObject : public CSE_ALifeObject
{
    using inherited = CSE_ALifeObject;

public:
    ALife::_TIME_ID m_tTimeID;
    u64 m_switch_counter;

    CSE_ALifeDynamicObject(LPCSTR caSection);
    virtual ~CSE_ALifeDynamicObject();
#ifdef XRGAME_EXPORTS
    virtual void on_spawn();
    virtual void on_before_register();
    virtual void on_register();
    virtual void on_unregister();
    virtual bool synchronize_location();
    virtual void try_switch_online();
    virtual void try_switch_offline();
    virtual void switch_online();
    virtual void switch_offline();
    virtual void add_online(const bool& update_registries);
    virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
    virtual bool redundant() const;
    void attach(CSE_ALifeInventoryItem* tpALifeInventoryItem, bool bALifeRequest, bool bAddChildren = true);
    void detach(CSE_ALifeInventoryItem* tpALifeInventoryItem, ALife::OBJECT_IT* I = 0, bool bALifeRequest = true,
        bool bRemoveChildren = true);
    virtual void clear_client_data();
    virtual void on_failed_switch_online();
#endif
    virtual CSE_ALifeDynamicObject* cast_alife_dynamic_object() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeDynamicObjectVisual : public CSE_ALifeDynamicObject, public CSE_Visual
{
    using inherited1 = CSE_ALifeDynamicObject;
    using inherited2 = CSE_Visual;

public:
    CSE_ALifeDynamicObjectVisual(LPCSTR caSection);
    virtual ~CSE_ALifeDynamicObjectVisual();
    virtual CSE_Visual* __stdcall visual();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifePHSkeletonObject : public CSE_ALifeDynamicObjectVisual, public CSE_PHSkeleton
{
    using inherited1 = CSE_ALifeDynamicObjectVisual;
    using inherited2 = CSE_PHSkeleton;

public:
    CSE_ALifePHSkeletonObject(LPCSTR caSection);
    virtual ~CSE_ALifePHSkeletonObject();
    virtual bool can_save() const /* noexcept */;
    virtual bool used_ai_locations() const /* noexcept */;
    virtual void load(NET_Packet& tNetPacket);
    virtual CSE_Abstract* cast_abstract() { return this; }
public:
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeSpaceRestrictor : public CSE_ALifeDynamicObject, public CSE_Shape
{
    using inherited1 = CSE_ALifeDynamicObject;
    using inherited2 = CSE_Shape;

public:
    u8 m_space_restrictor_type;

    CSE_ALifeSpaceRestrictor(LPCSTR caSection);
    virtual ~CSE_ALifeSpaceRestrictor();
    virtual IServerEntityShape* __stdcall shape();
    virtual bool can_switch_offline() const /* noexcept */;
    virtual bool used_ai_locations() const /* noexcept */;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeLevelChanger : public CSE_ALifeSpaceRestrictor
{
    using inherited = CSE_ALifeSpaceRestrictor;

public:
    GameGraph::_GRAPH_ID m_tNextGraphID;
    u32 m_dwNextNodeID;
    Fvector m_tNextPosition;
    Fvector m_tAngles;
    shared_str m_caLevelToChange;
    shared_str m_caLevelPointToChange;
    BOOL m_bSilentMode;

    CSE_ALifeLevelChanger(LPCSTR caSection);
    virtual ~CSE_ALifeLevelChanger();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeSmartZone : public CSE_ALifeSpaceRestrictor, public CSE_ALifeSchedulable
{
    using inherited1 = CSE_ALifeSpaceRestrictor;
    using inherited2 = CSE_ALifeSchedulable;

public:
    CSE_ALifeSmartZone(LPCSTR caSection);
    virtual ~CSE_ALifeSmartZone();
    virtual CSE_Abstract* base();
    virtual const CSE_Abstract* base() const;
    virtual CSE_Abstract* init();
    virtual CSE_Abstract* cast_abstract() { return this; };
    virtual CSE_ALifeSchedulable* cast_schedulable() { return this; };
    virtual void update();
    virtual float detect_probability();
    virtual void smart_touch(CSE_ALifeMonsterAbstract* monster);
    virtual bool used_ai_locations() const /* noexcept */ { return true; };
    virtual CSE_ALifeSmartZone* cast_smart_zone() { return this; };
#ifdef XRGAME_EXPORTS
    virtual bool bfActive();
    virtual CSE_ALifeItemWeapon* tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower);
    virtual CSE_ALifeDynamicObject* tpfGetBestDetector();
    virtual ALife::EMeetActionType tfGetActionType(
        CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex, bool bMutualDetection);
    // additional functionality
    virtual bool enabled(CSE_ALifeMonsterAbstract* object) const { return false; };
    virtual float suitable(CSE_ALifeMonsterAbstract* object) const { return 0.f; };
    virtual void register_npc(CSE_ALifeMonsterAbstract* object){};
    virtual void unregister_npc(CSE_ALifeMonsterAbstract* object){};
    virtual CALifeSmartTerrainTask* task(CSE_ALifeMonsterAbstract* object) { return 0; };
#endif
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeObjectPhysic : public CSE_ALifeDynamicObjectVisual, public CSE_PHSkeleton
{
    using inherited1 = CSE_ALifeDynamicObjectVisual;
    using inherited2 = CSE_PHSkeleton;

public:
    u32 type;
    f32 mass;
    shared_str fixed_bones;
    CSE_ALifeObjectPhysic(LPCSTR caSection);
    virtual ~CSE_ALifeObjectPhysic();
    virtual bool used_ai_locations() const /* noexcept */;
    virtual bool can_save() const /* noexcept */;
    virtual void load(NET_Packet& tNetPacket);
    virtual CSE_Abstract* cast_abstract() { return this; }
    //  virtual void                    load                    (IReader& r){inherited::load(r);}
    //  using inherited::load(IReader&);
private:
    u32 m_freeze_time;
    static const u32 m_freeze_delta_time;
#ifdef DEBUG // only for testing interpolation
    u32 m_last_update_time;
    static const u32 m_update_delta_time;
#endif
    static const u32 random_limit;
    CRandom m_relevent_random;

public:
    enum
    {
        inventory_item_state_enabled = u8(1) << 0,
        inventory_item_angular_null = u8(1) << 1,
        inventory_item_linear_null = u8(1) << 2 //,
        // animated                      = u8(1) << 3
    };
    union mask_num_items
    {
        struct
        {
            u8 num_items : 5;
            u8 mask : 3;
        };
        u8 common;
    };
    /////////// network ///////////////
    u8 m_u8NumItems;
    bool prev_freezed;
    bool freezed;
    SPHNetState State;

    virtual BOOL Net_Relevant();

    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeObjectHangingLamp : public CSE_ALifeDynamicObjectVisual, public CSE_PHSkeleton
{
    using inherited1 = CSE_ALifeDynamicObjectVisual;
    using inherited2 = CSE_PHSkeleton;

public:
    void __stdcall OnChangeFlag(PropValue* sender);
    enum
    {
        flPhysic = (1 << 0),
        flCastShadow = (1 << 1),
        flR1 = (1 << 2),
        flR2 = (1 << 3),
        flTypeSpot = (1 << 4),
        flPointAmbient = (1 << 5),
        flVolumetric = (1 << 6),
    };

    Flags16 flags;
    // light color
    u32 color;
    float brightness;
    shared_str color_animator;
    // light texture
    shared_str light_texture;
    // range
    float range;
    float m_virtual_size;
    // bones&motions
    shared_str light_ambient_bone;
    shared_str light_main_bone;
    shared_str fixed_bones;
    // spot
    float spot_cone_angle;
    // ambient
    float m_ambient_radius;
    float m_ambient_power;
    shared_str m_ambient_texture;
    //  volumetric
    float m_volumetric_quality;
    float m_volumetric_intensity;
    float m_volumetric_distance;
    // glow
    shared_str glow_texture;
    float glow_radius;
    // game
    float m_health;

    CSE_ALifeObjectHangingLamp(LPCSTR caSection);
    virtual ~CSE_ALifeObjectHangingLamp();
    virtual void load(NET_Packet& tNetPacket);
    virtual bool used_ai_locations() const /* noexcept */;
    virtual bool match_configuration() const /* noexcept */;
    virtual bool __stdcall validate();
#ifndef XRGAME_EXPORTS
    virtual void __stdcall on_render(CDUInterface* du, IServerEntityLEOwner* owner, bool bSelected,
        const Fmatrix& parent, int priority, bool strictB2F);
#endif // #ifndef XRGAME_EXPORTS
    virtual CSE_Abstract* cast_abstract() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeObjectProjector : public CSE_ALifeDynamicObjectVisual
{
    using inherited = CSE_ALifeDynamicObjectVisual;

public:
    CSE_ALifeObjectProjector(LPCSTR caSection);
    virtual ~CSE_ALifeObjectProjector();
    virtual bool used_ai_locations() const /* noexcept */;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeHelicopter : public CSE_ALifeDynamicObjectVisual, public CSE_Motion, public CSE_PHSkeleton
{
    using inherited1 = CSE_ALifeDynamicObjectVisual;
    using inherited2 = CSE_Motion;
    using inherited3 = CSE_PHSkeleton;

public:
    shared_str engine_sound;
    CSE_ALifeHelicopter(LPCSTR caSection);
    virtual ~CSE_ALifeHelicopter();
    virtual void load(NET_Packet& tNetPacket);
    virtual bool can_save() const /* noexcept */;
    virtual bool used_ai_locations() const /* noexcept */;
    virtual CSE_Motion* __stdcall motion();
    virtual CSE_Abstract* cast_abstract() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeCar : public CSE_ALifeDynamicObjectVisual, public CSE_PHSkeleton
{
    using inherited1 = CSE_ALifeDynamicObjectVisual;
    using inherited2 = CSE_PHSkeleton;

public:
    struct SDoorState
    {
        void read(NET_Packet& P);
        void write(NET_Packet& P);
        u8 open_state;
        float health;
    };
    struct SWheelState
    {
        void read(NET_Packet& P);
        void write(NET_Packet& P);
        float health;
    };
    xr_vector<SDoorState> door_states;
    xr_vector<SWheelState> wheel_states;
    float health;
    CSE_ALifeCar(LPCSTR caSection);
    virtual ~CSE_ALifeCar();
    virtual bool used_ai_locations() const /* noexcept */;
    virtual void load(NET_Packet& tNetPacket);
    virtual bool can_save() const /* noexcept */;
    virtual CSE_Abstract* cast_abstract() { return this; }
protected:
    virtual void data_load(NET_Packet& tNetPacket);
    virtual void data_save(NET_Packet& tNetPacket);

public:
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeObjectBreakable : public CSE_ALifeDynamicObjectVisual
{
    typedef CSE_ALifeDynamicObjectVisual inherited;

public:
    float m_health;
    CSE_ALifeObjectBreakable(LPCSTR caSection);
    virtual ~CSE_ALifeObjectBreakable();
    virtual bool used_ai_locations() const /* noexcept */;
    virtual bool can_switch_offline() const /* noexcept */;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeObjectClimable : public CSE_Shape, public CSE_ALifeDynamicObject
{
    using inherited1 = CSE_Shape;
    using inherited2 = CSE_ALifeDynamicObject;

public:
    CSE_ALifeObjectClimable(LPCSTR caSection);
    shared_str material;
    virtual ~CSE_ALifeObjectClimable();
    virtual bool used_ai_locations() const /* noexcept */;
    virtual bool can_switch_offline() const /* noexcept */;
    virtual IServerEntityShape* __stdcall shape();

#ifndef XRGAME_EXPORTS
    virtual void __stdcall set_additional_info(void* info);
#endif
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeMountedWeapon : public CSE_ALifeDynamicObjectVisual
{
    using inherited = CSE_ALifeDynamicObjectVisual;

public:
    CSE_ALifeMountedWeapon(LPCSTR caSection);
    virtual ~CSE_ALifeMountedWeapon();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeStationaryMgun : public CSE_ALifeDynamicObjectVisual
{
    using inherited = CSE_ALifeDynamicObjectVisual;

public:
    bool m_bWorking;
    Fvector m_destEnemyDir;

    CSE_ALifeStationaryMgun(LPCSTR caSection);
    virtual ~CSE_ALifeStationaryMgun();

    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeTeamBaseZone : public CSE_ALifeSpaceRestrictor
{
    using inherited = CSE_ALifeSpaceRestrictor;

public:
    CSE_ALifeTeamBaseZone(LPCSTR caSection);
    virtual ~CSE_ALifeTeamBaseZone();

    u8 m_team;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeInventoryBox : public CSE_ALifeDynamicObjectVisual
{
    using inherited = CSE_ALifeDynamicObjectVisual;

public:
    bool m_can_take;
    bool m_closed;
    shared_str m_tip_text;

    CSE_ALifeInventoryBox(LPCSTR caSection);
    virtual ~CSE_ALifeInventoryBox();
#ifdef XRGAME_EXPORTS
    virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
    virtual void add_online(const bool& update_registries);
#endif
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

#pragma warning(pop)

#endif
