#pragma once
#ifndef BoneH
#define BoneH

#include "xrCore/_obb.h"
#include "xrCore/_sphere.h"
#include "xrCore/_cylinder.h"
#include "xrCore/_flags.h"
#include "xrCore/FixedVector.h"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_vector.h"

// refs
class CBone;
class IReader;
class IWriter;

#define BI_NONE (u16(-1))

#define OGF_IKDATA_VERSION 0x0001

#define MAX_BONE_PARAMS 4

class XRCORE_API CBoneInstance;
// callback
using BoneCallbackFunction = void(CBoneInstance* P);
using BoneCallback = BoneCallbackFunction*;
// typedef void (* BoneCallback) (CBoneInstance* P);

//*** Bone Instance *******************************************************************************
#pragma pack(push, 8)
class XRCORE_API CBoneInstance
{
public:
    // data
    Fmatrix mTransform; // final x-form matrix (local to model)
    Fmatrix mRenderTransform; // final x-form matrix (model_base -> bone -> model)
private:
    BoneCallback Callback;
    void* Callback_Param;
    BOOL Callback_overwrite; // performance hint - don't calc anims
    u32 Callback_type;

public:
    float param[MAX_BONE_PARAMS]; //
    //
    // methods
public:
    IC BoneCallback callback() { return Callback; }
    IC void* callback_param() { return Callback_Param; }
    IC BOOL callback_overwrite() { return Callback_overwrite; } // performance hint - don't calc anims
    IC u32 callback_type() { return Callback_type; }
public:
    IC void construct();

    void set_callback(u32 Type, BoneCallback C, void* Param, BOOL overwrite = FALSE)
    {
        Callback = C;
        Callback_Param = Param;
        Callback_overwrite = overwrite;
        Callback_type = Type;
    }

    void reset_callback()
    {
        Callback = 0;
        Callback_Param = 0;
        Callback_overwrite = FALSE;
        Callback_type = 0;
    }
    void set_callback_overwrite(BOOL v) { Callback_overwrite = v; }
    void set_param(u32 idx, float data);
    float get_param(u32 idx);

    u32 mem_usage() { return sizeof(*this); }
};
#pragma pack(pop)

#pragma pack(push, 2)
struct XRCORE_API vertBoned1W // (3+3+3+3+2+1)*4 = 15*4 = 60 bytes
{
    Fvector P;
    Fvector N;
    Fvector T;
    Fvector B;
    float u, v;
    u32 matrix;
    void get_pos(Fvector& p) const { p.set(P); }
#ifdef DEBUG
    static const u8 bones_count = 1;
    u16 get_bone_id(u8 bone) const
    {
        VERIFY(bone < bones_count);
        return u16(matrix);
    }
#endif
};
struct XRCORE_API vertBoned2W // (1+3+3 + 1+3+3 + 2)*4 = 16*4 = 64 bytes
{
    u16 matrix0;
    u16 matrix1;
    Fvector P;
    Fvector N;
    Fvector T;
    Fvector B;
    float w;
    float u, v;
    void get_pos(Fvector& p) const { p.set(P); }
#ifdef DEBUG
    static const u8 bones_count = 2;
    u16 get_bone_id(u8 bone) const
    {
        VERIFY(bone < bones_count);
        return bone == 0 ? matrix0 : matrix1;
    }
#endif
};
struct XRCORE_API vertBoned3W // 70 bytes
{
    u16 m[3];
    Fvector P;
    Fvector N;
    Fvector T;
    Fvector B;
    float w[2];
    float u, v;
    void get_pos(Fvector& p) const { p.set(P); }
#ifdef DEBUG
    static const u8 bones_count = 3;
    u16 get_bone_id(u8 bone) const
    {
        VERIFY(bone < bones_count);
        return m[bone];
    }
#endif
};
struct XRCORE_API vertBoned4W // 76 bytes
{
    u16 m[4];
    Fvector P;
    Fvector N;
    Fvector T;
    Fvector B;
    float w[3];
    float u, v;
    void get_pos(Fvector& p) const { p.set(P); }
#ifdef DEBUG
    static const u8 bones_count = 4;
    u16 get_bone_id(u8 bone) const
    {
        VERIFY(bone < bones_count);
        return m[bone];
    }
#endif
};
#pragma pack(pop)

#pragma pack(push, 1)
enum EJointType
{
    jtRigid,
    jtCloth,
    jtJoint,
    jtWheel,
    jtNone,
    jtSlider,
    jtForceU32 = u32(-1)
};

struct XRCORE_API SJointLimit
{
    Fvector2 limit;
    float spring_factor;
    float damping_factor;
    SJointLimit() { Reset(); }
    void Reset()
    {
        limit.set(0.f, 0.f);
        spring_factor = 1.f;
        damping_factor = 1.f;
    }
};

struct XRCORE_API SBoneShape
{
    enum EShapeType
    {
        stNone,
        stBox,
        stSphere,
        stCylinder,
        stForceU32 = u16(-1)
    };

    enum EShapeFlags
    {
        sfNoPickable = (1 << 0), // use only in RayPick
        sfRemoveAfterBreak = (1 << 1),
        sfNoPhysics = (1 << 2),
        sfNoFogCollider = (1 << 3),
    };

    u16 type; // 2
    Flags16 flags; // 2
    Fobb box; // 15*4
    Fsphere sphere; // 4*4
    Fcylinder cylinder; // 8*4
    SBoneShape() { Reset(); }
    void Reset()
    {
        flags.zero();
        type = stNone;
        box.invalidate();
        sphere.P.set(0.f, 0.f, 0.f);
        sphere.R = 0.f;
        cylinder.invalidate();
    }
	bool Valid();
};

struct XRCORE_API SJointIKData
{
    // IK
    EJointType type;
    SJointLimit limits[3]; // by [axis XYZ on joint] and[Z-wheel,X-steer on wheel]
    float spring_factor;
    float damping_factor;
    enum
    {
        flBreakable = (1 << 0),
    };
    Flags32 ik_flags;
    float break_force; // [0..+INF]
    float break_torque; // [0..+INF]

    float friction;

    SJointIKData() { Reset(); }
    void Reset()
    {
        limits[0].Reset();
        limits[1].Reset();
        limits[2].Reset();
        type = jtRigid;
        spring_factor = 1.f;
        damping_factor = 1.f;
        ik_flags.zero();
        break_force = 0.f;
        break_torque = 0.f;
    }
    void clamp_by_limits(Fvector& dest_xyz);
    void Export(IWriter& F);
    bool Import(IReader& F, u16 vers);
};
#pragma pack(pop)

class XRCORE_API IBoneData
{
public:
    virtual IBoneData& GetChild(u16 id) = 0;
    virtual const IBoneData& GetChild(u16 id) const = 0;
    virtual u16 GetSelfID() const = 0;
    virtual u16 GetNumChildren() const = 0;

    virtual const SJointIKData& get_IK_data() const = 0;
    virtual const Fmatrix& get_bind_transform() const = 0;
    virtual const SBoneShape& get_shape() const = 0;
    virtual const Fobb& get_obb() const = 0;
    virtual const Fvector& get_center_of_mass() const = 0;
    virtual float get_mass() const = 0;
    virtual u16 get_game_mtl_idx() const = 0;
    virtual shared_str GetMaterialName() const = 0;
    virtual u16 GetParentID() const = 0;
    virtual float lo_limit(u8 k) const = 0;
    virtual float hi_limit(u8 k) const = 0;
};

// static const Fobb dummy ;//= Fobb().identity();
// refs
class CBone;
using BoneVec = xr_vector<CBone*>;

class XRCORE_API CBone : public CBoneInstance, public IBoneData
{
public:
    friend class LWBoneParser;

private:
    shared_str name;
    shared_str parent_name;
    shared_str wmap;
    Fvector rest_offset;
    Fvector rest_rotate; // XYZ format (Game format)
    float rest_length;

    Fvector mot_offset;
    Fvector mot_rotate; // XYZ format (Game format)
    float mot_length;

    Fmatrix mot_transform;

    Fmatrix local_rest_transform;
    Fmatrix rest_transform;
    Fmatrix rest_i_transform;

    // Fmatrix last_transform;

    // Fmatrix render_transform;
public:
    int SelfID;
    CBone* parent;
    BoneVec children;

public:
    // editor part
    Flags8 flags;
    enum
    {
        flSelected = (1 << 0),
    };
    SJointIKData IK_data;
    shared_str game_mtl;
    SBoneShape shape;

    float mass;
    Fvector center_of_mass;

public:
    CBone();
    virtual ~CBone();

    void SetName(const char* p)
    {
        name = p;
        xr_strlwr(name);
    }
    void SetParentName(const char* p)
    {
        parent_name = p;
        xr_strlwr(parent_name);
    }
    void SetWMap(const char* p) { wmap = p; }
    void SetRestParams(float length, const Fvector& offset, const Fvector& rotate)
    {
        rest_offset.set(offset);
        rest_rotate.set(rotate);
        rest_length = length;
    };

    shared_str Name() { return name; }
    shared_str ParentName() { return parent_name; }
    shared_str WMap() { return wmap; }
    IC CBone* Parent() { return parent; }
    IC BOOL IsRoot() { return (parent == 0); }
    shared_str& NameRef() { return name; }
    // transformation
    const Fvector& _Offset() { return mot_offset; }
    const Fvector& _Rotate() { return mot_rotate; }
    float _Length() { return mot_length; }
    IC Fmatrix& _RTransform() { return rest_transform; }
    IC Fmatrix& _RITransform() { return rest_i_transform; }
    IC Fmatrix& _LRTransform() { return local_rest_transform; }
    IC Fmatrix& _MTransform() { return mot_transform; }
    IC Fmatrix& _LTransform() { return mTransform; } //{return last_transform;}
    IC const Fmatrix& _LTransform() const { return mTransform; }
    IC Fmatrix& _RenderTransform() { return mRenderTransform; } //{return render_transform;}
    IC Fvector& _RestOffset() { return rest_offset; }
    IC Fvector& _RestRotate() { return rest_rotate; }
    void _Update(const Fvector& T, const Fvector& R)
    {
        mot_offset.set(T);
        mot_rotate.set(R);
        mot_length = rest_length;
    }
    void Reset()
    {
        mot_offset.set(rest_offset);
        mot_rotate.set(rest_rotate);
        mot_length = rest_length;
    }

    // IO
    void Save(IWriter& F);
    void Load_0(IReader& F);
    void Load_1(IReader& F);
    IC float engine_lo_limit(u8 k) const { return -IK_data.limits[k].limit.y; }
    IC float engine_hi_limit(u8 k) const { return -IK_data.limits[k].limit.x; }
    IC float editor_lo_limit(u8 k) const { return IK_data.limits[k].limit.x; }
    IC float editor_hi_limit(u8 k) const { return IK_data.limits[k].limit.y; }
    void SaveData(IWriter& F);
    void LoadData(IReader& F);
    void ResetData();
    void CopyData(CBone* bone);

    void ShapeScale(const Fvector& amount, bool parentCS = false);
    void ShapeRotate(const Fvector& amount, bool parentCS = false);
    void ShapeMove(const Fvector& amount, bool parentCS = false);
    void BindRotate(const Fvector& amount);
    void BindMove(const Fvector& amount);
    void BoneMove(const Fvector& amount);
    void BoneRotate(const Fvector& axis, float angle, bool parentCS = false);

    bool Pick(float& dist, const Fvector& S, const Fvector& D, const Fmatrix& parent);

    void Select(BOOL flag) { flags.set(flSelected, flag); }
    bool Selected() { return flags.is(flSelected); }
    void ClampByLimits();

    bool ExportOGF(IWriter& F);

private:
    IBoneData& GetChild(u16 id) { return *children[id]; }
    const IBoneData& GetChild(u16 id) const { return *children[id]; }
    u16 GetSelfID() const { return (u16)SelfID; }
    u16 GetNumChildren() const { return u16(children.size()); }
    const SJointIKData& get_IK_data() const { return IK_data; }
    const Fmatrix& get_bind_transform() const { return local_rest_transform; }
    const SBoneShape& get_shape() const { return shape; }
    const Fobb& get_obb() const;
    const Fvector& get_center_of_mass() const { return center_of_mass; }
    float get_mass() const { return mass; }
    // the caller should use GMLib.GetMaterialIdx instead
    virtual u16 get_game_mtl_idx() const override { return u16(-1); }
    virtual shared_str GetMaterialName() const override { return game_mtl; }
    u16 GetParentID() const
    {
        if (parent)
            return u16(parent->SelfID);
        else
            return u16(-1);
    };
    float lo_limit(u8 k) const { return engine_lo_limit(k); }
    float hi_limit(u8 k) const { return engine_hi_limit(k); }
};

//*** Shared Bone Data ****************************************************************************
class CBoneData;
// t-defs
typedef xr_vector<CBoneData*> vecBones;
typedef vecBones::iterator vecBonesIt;

class XRCORE_API CBoneData : public IBoneData
{
protected:
    u16 SelfID;
    u16 ParentID;

public:
    shared_str name;

    Fobb obb;

    Fmatrix bind_transform;
    Fmatrix m2b_transform; // model to bone conversion transform
    SBoneShape shape;
    shared_str game_mtl_name;
    u16 game_mtl_idx;
    SJointIKData IK_data;
    float mass;
    Fvector center_of_mass;

    vecBones children; // bones which are slaves to this

    using FacesVec = xr_vector<u16>;
    using ChildFacesVec = xr_vector<FacesVec>;
    ChildFacesVec child_faces; // shared

    CBoneData(u16 ID) : SelfID(ID) { VERIFY(SelfID != BI_NONE); }
    virtual ~CBoneData() {}
#ifdef DEBUG
    typedef svector<int, 128> BoneDebug;
    void DebugQuery(BoneDebug& L);
#endif
    IC void SetParentID(u16 id) { ParentID = id; }
    IC u16 GetSelfID() const { return SelfID; }
    IC u16 GetParentID() const { return ParentID; }
    // assign face
    void AppendFace(u16 child_idx, u16 idx) { child_faces[child_idx].push_back(idx); }
    // Calculation
    void CalculateM2B(const Fmatrix& Parent);

private:
    IBoneData& GetChild(u16 id) { return *children[id]; }
    const IBoneData& GetChild(u16 id) const { return *children[id]; }
    u16 GetNumChildren() const { return (u16)children.size(); }
    const SJointIKData& get_IK_data() const { return IK_data; }
    const Fmatrix& get_bind_transform() const { return bind_transform; }
    const SBoneShape& get_shape() const { return shape; }
    const Fobb& get_obb() const { return obb; }
    const Fvector& get_center_of_mass() const { return center_of_mass; }
    float get_mass() const { return mass; }
    virtual u16 get_game_mtl_idx() const override { return game_mtl_idx; }
    virtual shared_str GetMaterialName() const override { return name; }
    float lo_limit(u8 k) const { return IK_data.limits[k].limit.x; }
    float hi_limit(u8 k) const { return IK_data.limits[k].limit.y; }

public:
    virtual size_t mem_usage()
    {
        size_t sz = sizeof(*this) + sizeof(vecBones::value_type) * children.size();
        for (auto c_it = child_faces.begin(); c_it != child_faces.end(); ++c_it)
            sz += c_it->size() * sizeof(FacesVec::value_type) + sizeof(*c_it);
        return sz;
    }
};

enum EBoneCallbackType
{
    bctDummy = u32(0), // 0 - required!!!
    bctPhysics,
    bctCustom,
    bctForceU32 = u32(-1),
};

IC void CBoneInstance::construct()
{
    mTransform.identity();
    mRenderTransform.identity();

    Callback = NULL;
    Callback_Param = NULL;
    Callback_overwrite = FALSE;
    Callback_type = 0;

    ZeroMemory(&param, sizeof(param));
}

#endif
