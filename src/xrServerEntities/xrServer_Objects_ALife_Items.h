#pragma once
////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects items for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALife_ItemsH
#define xrServer_Objects_ALife_ItemsH

#include "xrServer_Objects_ALife.h"
#include "PHSynchronize.h"
#include "inventory_space.h"

#include "character_info_defs.h"
#include "InfoPortionDefs.h"

#pragma warning(push)
#pragma warning(disable : 4005)

class CSE_ALifeItemAmmo;

class CSE_ALifeInventoryItem
{
public:
    enum
    {
        inventory_item_state_enabled = u8(1) << 0,
        inventory_item_angular_null = u8(1) << 1,
        inventory_item_linear_null = u8(1) << 2,
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

public:
    float m_fCondition;
    float m_fMass;
    u32 m_dwCost;
    s32 m_iHealthValue;
    s32 m_iFoodValue;
    float m_fDeteriorationValue;
    CSE_ALifeObject* m_self;
    u32 m_last_update_time;
    xr_vector<shared_str> m_upgrades;

public:
    CSE_ALifeInventoryItem(LPCSTR caSection);
    virtual ~CSE_ALifeInventoryItem();
    // we need this to prevent virtual inheritance :-(
    virtual CSE_Abstract* base() = 0;
    virtual const CSE_Abstract* base() const = 0;
    virtual CSE_Abstract* init();
    virtual CSE_Abstract* cast_abstract() { return nullptr; };
    virtual CSE_ALifeInventoryItem* cast_inventory_item() { return this; };
    virtual u32 update_rate() const;
    virtual BOOL Net_Relevant();

    bool has_upgrade(const shared_str& upgrade_id);
    void add_upgrade(const shared_str& upgrade_id);

private:
    bool prev_freezed;
    bool freezed;
    u32 m_freeze_time;
    static const u32 m_freeze_delta_time;
    static const u32 random_limit;
    CRandom m_relevent_random;

public:
    // end of the virtual inheritance dependant code

    IC bool attached() const { return (base()->ID_Parent < 0xffff); }
    virtual bool bfUseful();

    /////////// network ///////////////
    u8 m_u8NumItems;
    SPHNetState State;
    ///////////////////////////////////
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItem : public CSE_ALifeDynamicObjectVisual, public CSE_ALifeInventoryItem
{
    using inherited1 = CSE_ALifeDynamicObjectVisual;
    using inherited2 = CSE_ALifeInventoryItem;

public:
    bool m_physics_disabled;

    CSE_ALifeItem(LPCSTR caSection);
    virtual ~CSE_ALifeItem();
    virtual CSE_Abstract* base();
    virtual const CSE_Abstract* base() const;
    virtual CSE_Abstract* init();
    virtual CSE_Abstract* cast_abstract() { return this; };
    virtual CSE_ALifeInventoryItem* cast_inventory_item() { return this; };
    virtual BOOL Net_Relevant();
    virtual void OnEvent(NET_Packet& tNetPacket, u16 type, u32 time, ClientID sender);
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemTorch : public CSE_ALifeItem
{
    typedef CSE_ALifeItem inherited;

public:
    enum EStats
    {
        eTorchActive = (1 << 0),
        eNightVisionActive = (1 << 1),
        eAttached = (1 << 2)
    };
    bool m_active;
    bool m_nightvision_active;
    bool m_attached;
    CSE_ALifeItemTorch(LPCSTR caSection);
    virtual ~CSE_ALifeItemTorch();
    virtual BOOL Net_Relevant();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemAmmo : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    u16 a_elapsed;
    u16 m_boxSize;

    CSE_ALifeItemAmmo(LPCSTR caSection);
    virtual ~CSE_ALifeItemAmmo();
    virtual CSE_ALifeItemAmmo* cast_item_ammo() { return this; };
    virtual bool can_switch_online() const /* noexcept */;
    virtual bool can_switch_offline() const /* noexcept */;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemWeapon : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    typedef ALife::EWeaponAddonStatus EWeaponAddonStatus;

    //текущее состояние аддонов
    enum EWeaponAddonState
    {
        eWeaponAddonScope = 0x01,
        eWeaponAddonGrenadeLauncher = 0x02,
        eWeaponAddonSilencer = 0x04
    };

    EWeaponAddonStatus m_scope_status;
    EWeaponAddonStatus m_silencer_status;
    EWeaponAddonStatus m_grenade_launcher_status;

    u32 timestamp;
    u8 wpn_flags;
    u8 wpn_state;
    u8 ammo_type;
    u16 a_current;
    u16 a_elapsed;
    // count of grenades to spawn in grenade launcher [ttcccccc]
    // WARNING! hight 2 bits (tt bits) indicate type of grenade, so maximum grenade count is 2^6 = 64
    struct grenade_count_t
    {
        u8 grenades_count : 6;
        u8 grenades_type : 2;
        u8 pack_to_byte() const { return (grenades_type << 6) | grenades_count; }
        void unpack_from_byte(u8 const b)
        {
            grenades_type = (b >> 6);
            grenades_count = b & 0x3f; // 111111
        }
    }; // struct grenade_count_t
    grenade_count_t a_elapsed_grenades;

    float m_fHitPower;
    ALife::EHitType m_tHitType;
    LPCSTR m_caAmmoSections;
    u32 m_dwAmmoAvailable;
    Flags8 m_addon_flags;
    u8 m_bZoom;
    u32 m_ef_main_weapon_type;
    u32 m_ef_weapon_type;

    CSE_ALifeItemWeapon(LPCSTR caSection);
    virtual ~CSE_ALifeItemWeapon();
    virtual void OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender);
    virtual u32 ef_main_weapon_type() const;
    virtual u32 ef_weapon_type() const;
    u8 get_slot();
    u16 get_ammo_limit();
    u16 get_ammo_total();
    u16 get_ammo_elapsed();
    u16 get_ammo_magsize();
    void clone_addons(CSE_ALifeItemWeapon* parent);

    virtual BOOL Net_Relevant();

    virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemWeaponMagazined : public CSE_ALifeItemWeapon
{
    typedef CSE_ALifeItemWeapon inherited;

public:
    u8 m_u8CurFireMode;
    CSE_ALifeItemWeaponMagazined(LPCSTR caSection);
    virtual ~CSE_ALifeItemWeaponMagazined();

    virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemWeaponMagazinedWGL : public CSE_ALifeItemWeaponMagazined
{
    using inherited = CSE_ALifeItemWeaponMagazined;

public:
    bool m_bGrenadeMode;
    CSE_ALifeItemWeaponMagazinedWGL(LPCSTR caSection);
    virtual ~CSE_ALifeItemWeaponMagazinedWGL();

    virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemWeaponShotGun : public CSE_ALifeItemWeaponMagazined
{
    using inherited = CSE_ALifeItemWeaponMagazined;

public:
    xr_vector<u8> m_AmmoIDs;
    CSE_ALifeItemWeaponShotGun(LPCSTR caSection);
    virtual ~CSE_ALifeItemWeaponShotGun();

    virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemWeaponAutoShotGun : public CSE_ALifeItemWeaponShotGun
{
    using inherited = CSE_ALifeItemWeaponShotGun;

public:
    CSE_ALifeItemWeaponAutoShotGun(LPCSTR caSection);
    virtual ~CSE_ALifeItemWeaponAutoShotGun();

    virtual CSE_ALifeItemWeapon* cast_item_weapon() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemDetector : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    u32 m_ef_detector_type;
    CSE_ALifeItemDetector(LPCSTR caSection);
    virtual ~CSE_ALifeItemDetector();
    virtual u32 ef_detector_type() const;
    virtual CSE_ALifeItemDetector* cast_item_detector() { return this; }
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemArtefact : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    float m_fAnomalyValue;
    CSE_ALifeItemArtefact(LPCSTR caSection);
    virtual ~CSE_ALifeItemArtefact();
    virtual BOOL Net_Relevant();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemPDA : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    u16 m_original_owner;
    shared_str m_specific_character;
    shared_str m_info_portion;

    CSE_ALifeItemPDA(LPCSTR caSection);
    virtual ~CSE_ALifeItemPDA();
    virtual CSE_ALifeItemPDA* cast_item_pda() { return this; };
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemDocument : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    shared_str m_wDoc;
    CSE_ALifeItemDocument(LPCSTR caSection);
    virtual ~CSE_ALifeItemDocument();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemGrenade : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    u32 m_ef_weapon_type;
    CSE_ALifeItemGrenade(LPCSTR caSection);
    virtual ~CSE_ALifeItemGrenade();
    virtual u32 ef_weapon_type() const;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemExplosive : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    CSE_ALifeItemExplosive(LPCSTR caSection);
    virtual ~CSE_ALifeItemExplosive();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemBolt : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    u32 m_ef_weapon_type;
    CSE_ALifeItemBolt(LPCSTR caSection);
    virtual ~CSE_ALifeItemBolt();
    virtual bool can_save() const /* noexcept */;
    virtual bool used_ai_locations() const /* noexcept */;
    virtual u32 ef_weapon_type() const;
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemCustomOutfit : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    u32 m_ef_equipment_type;
    CSE_ALifeItemCustomOutfit(LPCSTR caSection);
    virtual ~CSE_ALifeItemCustomOutfit();
    virtual u32 ef_equipment_type() const;
    virtual BOOL Net_Relevant();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

class CSE_ALifeItemHelmet : public CSE_ALifeItem
{
    using inherited = CSE_ALifeItem;

public:
    CSE_ALifeItemHelmet(LPCSTR caSection);
    virtual ~CSE_ALifeItemHelmet();
    virtual BOOL Net_Relevant();
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};

#pragma warning(pop)

#endif
