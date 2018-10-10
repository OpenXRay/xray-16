////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_ALife_Smartcovers.h
//  Created     : 17.12.2008
//  Modified    :
//  Author      : Alexander Plichko
//  Description : Server objects smartcovers for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALife_SmartcoversH
#define xrServer_Objects_ALife_SmartcoversH

#include "xrServer_Objects_ALife.h"

#pragma warning(push)
#pragma warning(disable : 4005)

class CSE_ALifeDynamicObject;

class CSE_SmartCover : public CSE_ALifeDynamicObject, public CSE_Shape
{
    using inherited1 = CSE_ALifeDynamicObject;
    using inherited2 = CSE_Shape;

public:
    struct SSCDrawHelper
    {
        shared_str string_identifier;
        Fvector point_position;
        bool is_enterable;
        Fvector enter_direction;
        float fov;
        float range;
        Fvector fov_direction;
        shared_str animation_id;
    };
    xr_vector<SSCDrawHelper> m_draw_data;
    shared_str m_description;
    float m_hold_position_time;
    float m_enter_min_enemy_distance;
    float m_exit_min_enemy_distance;
    BOOL m_is_combat_cover;
    BOOL m_can_fire;
    bool m_need_to_reparse_loopholes;
#ifndef AI_COMPILER
    luabind::object m_available_loopholes;
#endif // #ifndef AI_COMPILER

#ifdef XRSE_FACTORY_EXPORTS
private:
    typedef xr_vector<visual_data> visuals_collection;

    void __stdcall OnChangeDescription(PropValue* sender);
    void __stdcall OnChangeLoopholes(PropValue* sender);

public:
    void set_loopholes_table_checker(BOOLValue* value);

private:
    mutable visuals_collection m_visuals;
#endif // #ifdef XRSE_FACTORY_EXPORTS

public:
    CSE_SmartCover(LPCSTR caSection);
    virtual ~CSE_SmartCover();
    virtual IServerEntityShape* __stdcall shape();
    virtual bool used_ai_locations() const /* noexcept */;
    virtual bool can_save() const /* noexcept */;
    virtual bool can_switch_online() const /* noexcept */;
    virtual bool can_switch_offline() const /* noexcept */;
    virtual bool interactive() const /* noexcept */;
    LPCSTR description() const;
#ifndef AI_COMPILER
    void set_available_loopholes(luabind::object table);
#endif // #ifndef AI_COMPILER
#ifdef XRSE_FACTORY_EXPORTS
    virtual void __stdcall on_render(CDUInterface* du, IServerEntityLEOwner* owner, bool bSelected,
        const Fmatrix& parent, int priority, bool strictB2F);
    virtual visual_data* __stdcall visual_collection() const { return &*m_visuals.begin(); }
    virtual u32 __stdcall visual_collection_size() const { return m_visuals.size(); }
#endif // #ifdef XRSE_FACTORY_EXPORTS

private:
    void check_enterable_loopholes(shared_str const& description);
    void set_enterable(shared_str const& id);
    void fill_visuals();
    void load_draw_data();

public:
    virtual void UPDATE_Read(NET_Packet& P);
    virtual void UPDATE_Write(NET_Packet& P);
    virtual void STATE_Read(NET_Packet& P, u16 size);
    virtual void STATE_Write(NET_Packet& P);
    SERVER_ENTITY_EDITOR_METHODS
};
#pragma warning(pop)
#endif
