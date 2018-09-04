#pragma once

#include "Common/Noncopyable.hpp"
#include "xrCore/xrPool.h"
//#include "xr_collide_defs.h"
#include "xrCore/xrCore_benchmark_macros.h"
#include "xrCore/_types.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_sphere.h"
#include "xrCore/FTimer.h"
#include "xrCDB.h"

#pragma pack(push, 4)

/*
Requirements:
0. Generic
    * O(1) insertion
        - radius completely determines	"level"
        - position completely determines "node"
    * O(1) removal
    *
1. Rendering
    * Should live inside spatial DB
    * Should have at least "bounding-sphere" or "bounding-box"
    * Should have pointer to "sector" it lives in
    * Approximate traversal order relative to point ("camera")
2. Spatial queries
    * Should live inside spatial DB
    * Should have at least "bounding-sphere" or "bounding-box"
*/

const float c_spatial_min = 8.f;
//////////////////////////////////////////////////////////////////////////
enum
{
    STYPE_RENDERABLE = (1 << 0),
    STYPE_LIGHTSOURCE = (1 << 1),
    STYPE_COLLIDEABLE = (1 << 2),
    STYPE_VISIBLEFORAI = (1 << 3),
    STYPE_REACTTOSOUND = (1 << 4),
    STYPE_PHYSIC = (1 << 5),
    STYPE_OBSTACLE = (1 << 6),
    STYPE_SHAPE = (1 << 7),
    STYPE_LIGHTSOURCEHEMI = (1 << 8),

    STYPEFLAG_INVALIDSECTOR = (1 << 16)
};
//////////////////////////////////////////////////////////////////////////
// Comment:
//		ordinal objects			- renderable?, collideable?, visibleforAI?
//		physical-decorations	- renderable, collideable
//		lights					- lightsource
//		particles(temp-objects)	- renderable
//		glow					- renderable
//		sound					- ???
//////////////////////////////////////////////////////////////////////////
// class 				IRender_Sector;
// class 				ISpatial;
// class 				ISpatial_NODE;
// class 				ISpatial_DB;

//////////////////////////////////////////////////////////////////////////
// Fast type conversion
// class 			IGameObject;
// class 			IRenderable;
// class 			IRender_Light;
//
// namespace Feel { class Sound; }

//////////////////////////////////////////////////////////////////////////
class ISpatial_NODE;
class IRender_Sector;
class ISpatial_DB;
class IGameObject;
namespace Feel
{
class Sound;
}
class IRenderable;
class IRender_Light;
class Lock;

class SpatialData
{
public:
    u32 type = 0; // STYPE_
    Fsphere sphere;
    Fvector node_center; // Cached node center for TBV optimization
    float node_radius; // Cached node bounds for TBV optimization
    ISpatial_NODE* node_ptr; // Cached parent node for "empty-members" optimization
    IRender_Sector* sector;
    ISpatial_DB* space; // allow different spaces
};

class ISpatial
{
public:
    virtual ~ISpatial() = 0;
    virtual SpatialData& GetSpatialData() = 0;
    virtual bool spatial_inside() = 0;
    virtual void spatial_register() = 0;
    virtual void spatial_unregister() = 0;
    virtual void spatial_move() = 0;
    virtual Fvector spatial_sector_point() = 0;
    virtual void spatial_updatesector() = 0;
    virtual IGameObject* dcast_GameObject() = 0;
    virtual Feel::Sound* dcast_FeelSound() = 0;
    virtual IRenderable* dcast_Renderable() = 0;
    virtual IRender_Light* dcast_Light() = 0;
};

ICF ISpatial::~ISpatial() {}
class XRCDB_API SpatialBase : public virtual ISpatial
{
public:
    SpatialData spatial;

private:
    void spatial_updatesector_internal();

public:
    virtual SpatialData& GetSpatialData() override final { return spatial; }
    virtual bool spatial_inside() override final;
    virtual void spatial_register() override;
    virtual void spatial_unregister() override;
    BENCH_SEC_SCRAMBLEVTBL2
    virtual void spatial_move() override;
    virtual Fvector spatial_sector_point() override { return spatial.sphere.P; }
    virtual void spatial_updatesector() override final
    {
        if (0 == (spatial.type & STYPEFLAG_INVALIDSECTOR))
            return;
        spatial_updatesector_internal();
    }

    virtual IGameObject* dcast_GameObject() override { return nullptr; }
    virtual Feel::Sound* dcast_FeelSound() override { return nullptr; }
    virtual IRenderable* dcast_Renderable() override { return nullptr; }
    virtual IRender_Light* dcast_Light() override { return nullptr; }
    SpatialBase(ISpatial_DB* space);
    virtual ~SpatialBase();
};

//////////////////////////////////////////////////////////////////////////
// class ISpatial_NODE;
class ISpatial_NODE
{
public:
    ISpatial_NODE* parent; // parent node for "empty-members" optimization
    ISpatial_NODE* children[8]; // children nodes
    xr_vector<ISpatial*> items; // own items

    void _init(ISpatial_NODE* _parent);
    void _remove(ISpatial* _S);
    void _insert(ISpatial* _S);
    bool _empty()
    {
        return items.empty() &&
            0 == (uintptr_t(children[0]) | uintptr_t(children[1]) |
                  uintptr_t(children[2]) | uintptr_t(children[3]) |
                  uintptr_t(children[4]) | uintptr_t(children[5]) |
                  uintptr_t(children[6]) | uintptr_t(children[7]));
    }
};

class XRCDB_API ISpatial_DB : private Noncopyable
{
public:
    struct SpatialDBStatistics
    {
        u32 NodeCount;
        u32 ObjectCount;
#ifdef DEBUG
        CStatTimer Insert; // debug only
        CStatTimer Remove; // debug only
#endif
        CStatTimer Query;

        SpatialDBStatistics()
        {
            NodeCount = ObjectCount = 0;
            FrameStart();
        }

        void FrameStart()
        {
#ifdef DEBUG
            Insert.FrameStart();
            Remove.FrameStart();
#endif
            Query.FrameStart();
        }

        void FrameEnd()
        {
#ifdef DEBUG
            Insert.FrameEnd();
            Remove.FrameEnd();
#endif
            Query.FrameEnd();
        }
    };

private:
    Lock* pcs;

    poolSS<ISpatial_NODE, 128> allocator;

    xr_vector<ISpatial_NODE*> allocator_pool;
    ISpatial* rt_insert_object;

public:
    char Name[64];
    ISpatial_NODE* m_root;
    Fvector m_center;
    float m_bounds;
    xr_vector<ISpatial*>* q_result;
    SpatialDBStatistics Stats;

private:
    friend class ISpatial_NODE;

    IC u32 _octant(u32 x, u32 y, u32 z) { return z * 4 + y * 2 + x; }
    IC u32 _octant(Fvector& base, Fvector& rel)
    {
        u32 o = 0;
        if (rel.x > base.x)
            o += 1;
        if (rel.y > base.y)
            o += 2;
        if (rel.z > base.z)
            o += 4;
        return o;
    }

    ISpatial_NODE* _node_create();
    void _node_destroy(ISpatial_NODE*& P);

    void _insert(ISpatial_NODE* N, Fvector& n_center, float n_radius);
    void _remove(ISpatial_NODE* N, ISpatial_NODE* N_sub);

public:
    ISpatial_DB(const char* name);
    ~ISpatial_DB();

    // managing
    void initialize(Fbox& BB);
    // void							destroy			();
    void insert(ISpatial* S);
    void remove(ISpatial* S);
    void update(u32 nodes = 8);
    BOOL verify();

    enum
    {
        O_ONLYFIRST = (1 << 0),
        O_ONLYNEAREST = (1 << 1),
        O_ORDERED = (1 << 2),
        O_force_u32 = u32(-1)
    };

    // query
    void q_ray(
        xr_vector<ISpatial*>& R, u32 _o, u32 _mask_and, const Fvector& _start, const Fvector& _dir, float _range);
    void q_box(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or, const Fvector& _center, const Fvector& _size);
    void q_sphere(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or, const Fvector& _center, const float _radius);
    void q_frustum(xr_vector<ISpatial*>& R, u32 _o, u32 _mask_or, const CFrustum& _frustum);
};

XRCDB_API extern ISpatial_DB* g_SpatialSpace;
XRCDB_API extern ISpatial_DB* g_SpatialSpacePhysic;

#pragma pack(pop)
