////////////////////////////////////////////////////////////////////////////
//	Module 		: attachable_item.h
//	Created 	: 11.02.2004
//  Modified 	: 11.02.2004
//	Author		: Dmitriy Iassenev
//	Description : Attachable item
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/_types.h"
#include "xrCore/_matrix.h"
#include "xrCore/xrstring.h"
#ifdef DEBUG
#include "xrCore/log.h"
#include "xrCore/_vector3d.h"
#endif

class CPhysicsShellHolder;
class CInventoryItem;
class IFactoryObject;

class CAttachableItem
{
private:
    CInventoryItem* m_item;
    shared_str m_bone_name;
    Fmatrix m_offset;
    u16 m_bone_id;
    bool m_enabled;
#ifdef DEBUG
    bool m_valid;
#endif

public:
    IC CAttachableItem();
    virtual ~CAttachableItem();
    virtual IFactoryObject* _construct();
    virtual CAttachableItem* cast_attachable_item() { return this; }
    virtual void reload(LPCSTR section);
    virtual void OnH_A_Chield();
    virtual void OnH_A_Independent();
    virtual void renderable_Render();
    virtual bool can_be_attached() const;
    bool load_attach_position(LPCSTR section);
    virtual void afterAttach();
    virtual void afterDetach();
    IC CInventoryItem& item() const;
    CPhysicsShellHolder& object() const;
    IC shared_str bone_name() const;
    IC u16 bone_id() const;
    IC void set_bone_id(u16 bone_id);
    IC const Fmatrix& offset() const;
    IC bool enabled() const;
    virtual void enable(bool value);

protected:
    virtual bool use_parent_ai_locations() const = 0;
public:
#ifdef DEBUG
    static CAttachableItem* m_dbgItem;
    static Fvector get_angle_offset()
    {
        VERIFY(m_dbgItem);
        Fvector v;
        m_dbgItem->m_offset.getHPB(v);
        return v;
    };
    static Fvector get_pos_offset()
    {
        VERIFY(m_dbgItem);
        return m_dbgItem->m_offset.c;
    };
    static void set_angle_offset(Fvector val)
    {
        Fvector c = get_pos_offset();
        m_dbgItem->m_offset.setHPB(VPUSH(val));
        m_dbgItem->m_offset.c = c;
    }

    static void rot(int axis, float val)
    {
        Fvector v = get_angle_offset();
        v[axis] += val;
        set_angle_offset(v);
    }
    static void rot_dx(float val)
    {
        Fvector v = get_angle_offset();
        v.x += val;
        set_angle_offset(v);
    }
    static void rot_dy(float val)
    {
        Fvector v = get_angle_offset();
        v.y += val;
        set_angle_offset(v);
    }
    static void rot_dz(float val)
    {
        Fvector v = get_angle_offset();
        v.z += val;
        set_angle_offset(v);
    }

    static void mov(int axis, float val)
    {
        Fvector c = get_pos_offset();
        c[axis] += val;
        m_dbgItem->m_offset.c = c;
    }
    static void mov_dx(float val)
    {
        Fvector c = get_pos_offset();
        c.x += val;
        m_dbgItem->m_offset.c = c;
    }
    static void mov_dy(float val)
    {
        Fvector c = get_pos_offset();
        c.y += val;
        m_dbgItem->m_offset.c = c;
    }
    static void mov_dz(float val)
    {
        Fvector c = get_pos_offset();
        c.z += val;
        m_dbgItem->m_offset.c = c;
    }
#endif
};

#include "attachable_item_inline.h"
