////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_Abstract.h
//  Created     : 19.09.2002
//  Modified    : 18.06.2004
//  Author      : Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//  Description : Server objects
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_AbstractH
#define xrServer_Objects_AbstractH

#pragma pack(push, 4)
#include "xrServer_Space.h"
#include "xrCDB/xrCDB.h"
#include "ShapeData.h"
#include "gametype_chooser.h"

class NET_Packet;
class CDUInterface;

#ifndef _EDITOR
#ifndef XRGAME_EXPORTS
#include "xrSound/Sound.h"
#endif
#endif

#include "xrEProps.h"
#include "Include/xrRender/DrawUtils.h"
#pragma warning(push)
#pragma warning(disable : 4005)

class IServerEntityShape
{
public:
    virtual ~IServerEntityShape() = 0;
    virtual void __stdcall assign_shapes(CShapeData::shape_def* shapes, u32 cnt) = 0;
};

IC IServerEntityShape::~IServerEntityShape() {}
class CSE_Visual
{
public:
    void __stdcall OnChangeVisual(PropValue* sender);
    void __stdcall OnChangeAnim(PropValue* sender);

public:
    shared_str visual_name;
    shared_str startup_animation;
    enum
    {
        flObstacle = (1 << 0)
    };
    Flags8 flags;

public:
    CSE_Visual(LPCSTR name = nullptr);
    virtual ~CSE_Visual();

    void visual_read(NET_Packet& P, u16 version);
    void visual_write(NET_Packet& P);

    void set_visual(LPCSTR name, bool load = true);
    LPCSTR get_visual() const { return *visual_name; }
#ifndef XRGAME_EXPORTS
    virtual void FillProps(LPCSTR pref, PropItemVec& items);
#endif // #ifndef XRGAME_EXPORTS

    virtual CSE_Visual* __stdcall visual() = 0;
};

class CSE_Motion
{
public:
    void __stdcall OnChangeMotion(PropValue* sender);

public:
    shared_str motion_name;

public:
    CSE_Motion(LPCSTR name = nullptr);
    virtual ~CSE_Motion();

    void motion_read(NET_Packet& P);
    void motion_write(NET_Packet& P);

    void set_motion(LPCSTR name);
    LPCSTR get_motion() const { return *motion_name; }
#ifndef XRGAME_EXPORTS
    virtual void FillProps(LPCSTR pref, PropItemVec& items);
#endif // #ifndef XRGAME_EXPORTS

    virtual CSE_Motion* __stdcall motion() = 0;
};

class IServerEntityLEOwner
{
public:
    virtual ~IServerEntityLEOwner() = 0;
    virtual void __stdcall get_bone_xform(LPCSTR name, Fmatrix& xform) = 0;
};

IC IServerEntityLEOwner::~IServerEntityLEOwner() {}
#pragma pack(push, 1)
class visual_data
{
public:
    Fmatrix matrix;
    CSE_Visual* visual;
};
#pragma pack(pop)

class IServerEntity
{
public:
    enum
    {
        flUpdateProperties = u32(1 << 0),
        flVisualChange = u32(1 << 1),
        flVisualAnimationChange = u32(1 << 2),
        flMotionChange = u32(1 << 3),
        flVisualAnimationPauseChange = u32(1 << 4),
    };

    virtual ~IServerEntity() = 0;
    // XXX: move to implementation
    Flags32 m_editor_flags;
    void set_editor_flag(u32 mask) { m_editor_flags.set(mask, true); }
public:
    virtual void __stdcall Spawn_Write(NET_Packet& tNetPacket, BOOL bLocal) = 0;
    virtual BOOL __stdcall Spawn_Read(NET_Packet& tNetPacket) = 0;
#ifndef XRGAME_EXPORTS
    virtual void __stdcall FillProp(LPCSTR pref, PropItemVec& items) = 0;
    virtual void __stdcall on_render(CDUInterface* du, IServerEntityLEOwner* owner, bool bSelected,
        const Fmatrix& parent, int priority, bool strictB2F) = 0;
    virtual visual_data* __stdcall visual_collection() const = 0;
    virtual u32 __stdcall visual_collection_size() const = 0;
    virtual void __stdcall set_additional_info(void* info) = 0;
#endif // #ifndef XRGAME_EXPORTS
    virtual LPCSTR __stdcall name() const = 0;
    virtual void __stdcall set_name(LPCSTR) = 0;
    virtual LPCSTR __stdcall name_replace() const = 0;
    virtual void __stdcall set_name_replace(LPCSTR) = 0;
    virtual Fvector& __stdcall position() = 0;
    virtual Fvector& __stdcall angle() = 0;
    virtual Flags16& __stdcall flags() = 0;
    virtual IServerEntityShape* __stdcall shape() = 0;
    virtual CSE_Visual* __stdcall visual() = 0;
    virtual CSE_Motion* __stdcall motion() = 0;
    virtual bool __stdcall validate() = 0;
};

IC IServerEntity::~IServerEntity() {}
#pragma warning(pop)

#pragma pack(pop)
#endif // xrServer_Objects_AbstractH
