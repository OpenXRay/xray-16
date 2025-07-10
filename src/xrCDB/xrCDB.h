#pragma once

#include "xrCore/Threading/Lock.hpp" // XXX: Remove from header. Put in .cpp.
#include "Common/Noncopyable.hpp"
#include "xrCore/math_constants.h"
#include "xrCore/_vector3d.h"
#include "xrCommon/xr_vector.h"

#ifdef XRAY_STATIC_BUILD
#   define XRCDB_API
#else
#   ifdef XRCDB_EXPORTS
#      define XRCDB_API XR_EXPORT
#   else
#      define XRCDB_API XR_IMPORT
#   endif
#endif

// forward declarations
class CFrustum;
namespace Opcode
{
class OPCODE_Model;
class AABBNoLeafNode;
};

struct Fbox3;
using Fbox = Fbox3;
class Lock;

#pragma pack(push, 8)
namespace CDB
{
// Triangle
class TRI //*** 16 bytes total (was 32 :)
{
public:
    u32 verts[3]; // 3*4 = 12b
    union
    {
        u32 dummy; // 4b
        struct
        {
            u32 material : 14; //
            u32 suppress_shadows : 1; //
            u32 suppress_wm : 1; //
            u32 sector : 16; //
        };
    };

public:
    [[nodiscard]]
    auto IDvert(const size_t ID) const { return verts[ID]; }
};

static_assert(std::is_trivial_v<TRI> && std::is_standard_layout_v<TRI>);
static_assert(sizeof(TRI) == 16, "TRI always should be 16 bytes on any architecture.");

// Build callback
using build_callback = void(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt, void* params);
using serialize_callback = void(IWriter& writer);
using deserialize_callback = bool(IReader& reader);
using remapping_materials_callback = void(TRI* T, u32 Tcnt, xr_map<u16, shared_str>& gameMtls);

// Model definition
class XRCDB_API MODEL : Noncopyable
{
    friend class COLLIDER;

    enum : u32
    {
        S_READY = 0,
        S_INIT = 1,
        S_BUILD = 2,
    };

private:
    Lock* pcs;
    Opcode::OPCODE_Model* tree{};
    volatile u32 status{ S_INIT }; // 0=ready, 1=init, 2=building
    u32 model_crc32{};

    // tris
    u32 tris_count{};
    u32 verts_count{};
    TRI* tris{};
    Fvector* verts{};

public:
    MODEL();
    ~MODEL();

    [[nodiscard]]
    auto get_verts_count() const { return verts_count; }
    [[nodiscard]]
    auto get_tris_count() const { return tris_count; }

    [[nodiscard]]
    const auto* get_verts() const { return verts; }
    [[nodiscard]]
    auto get_verts() { return verts; }
    [[nodiscard]]
    const auto* get_tris() const { return tris; }
    [[nodiscard]]
    auto get_tris() { return tris; }

    void syncronize() const
    {
        if (S_READY != status)
            syncronize_impl();
    }

    void build_internal(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt, build_callback* bc = nullptr, void* bcp = nullptr);
    void build(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt, build_callback* bc = nullptr, void* bcp = nullptr);

    void set_model_crc32(u32 value) { model_crc32 = value; }
    void load_geom(Fvector* V, u32 Vcnt, TRI* T, u32 Tcnt);
    bool serialize(pcstr fileName, serialize_callback callback = nullptr) const;
    bool deserialize(pcstr fileName, bool skipCrc32Check = false, deserialize_callback callback = nullptr);
    void deserialize_tree(IReader* rstream);

    size_t memory();

private:
    void syncronize_impl() const;
};

// Collider result
struct XRCDB_API RESULT
{
    Fvector verts[3];
    union
    {
        u32 dummy; // 4b
        struct
        {
            u32 material : 14; //
            u32 suppress_shadows : 1; //
            u32 suppress_wm : 1; //
            u32 sector : 16; //
        };
    };
    int id;
    float range;
    float u, v;
};

// Collider Options
enum
{
    OPT_CULL = (1 << 0),
    OPT_ONLYFIRST = (1 << 1),
    OPT_ONLYNEAREST = (1 << 2),
    OPT_FULL_TEST = (1 << 3) // for box & frustum queries - enable class III test(s)
};

// Collider itself
class XRCDB_API COLLIDER
{
    // Result management
    xr_vector<RESULT> rd;

public:
    ~COLLIDER();

    void ray_query(u32 ray_mode, const MODEL* m_def, const Fvector& r_start, const Fvector& r_dir, float r_range = 10000.f);
    void box_query(u32 box_mode, const MODEL* m_def, const Fvector& b_center, const Fvector& b_dim);
    void frustum_query(u32 frustum_mode, const MODEL* m_def, const CFrustum& F);

    ICF RESULT* r_begin() { return &*rd.begin(); };
    //ICF RESULT* r_end() { return &*rd.end(); };
    ICF xr_vector<RESULT>* r_get() { return &rd; };
    RESULT& r_add();
    void r_free();
    ICF size_t r_count() { return rd.size(); };
    ICF void r_clear() { rd.clear(); };
    ICF void r_clear_compact() { rd.clear(); };
};

//
class XRCDB_API Collector
{
    xr_vector<Fvector> verts;
    xr_vector<TRI> faces;

    u32 VPack(const Fvector& V, float eps);

public:
    void add_face(const Fvector& v0, const Fvector& v1, const Fvector& v2, u16 material, u16 sector);
    void add_face_D(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy);
    void add_face_packed(
        const Fvector& v0, const Fvector& v1, const Fvector& v2, u16 material, u16 sector, float eps = EPS);
    void add_face_packed_D(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy, float eps = EPS);
    void remove_duplicate_T();
    void calc_adjacency(xr_vector<u32>& dest) const;

    [[nodiscard]]
    auto getVS() const { return verts.size(); }
    [[nodiscard]]
    auto getTS() const { return faces.size(); }

    [[nodiscard]]
    auto getV() const { return &verts.front(); }
    [[nodiscard]]
    auto getT() const { return &faces.front(); }

    [[nodiscard]]
    auto getV() { return &verts.front(); }
    [[nodiscard]]
    auto getT() { return &faces.front(); }

    void clear()
    {
        verts.clear();
        faces.clear();
    }
};

#pragma warning(push)
#pragma warning(disable : 4275)
const u32 clpMX = 24, clpMY = 16, clpMZ = 24;
class XRCDB_API CollectorPacked : public Noncopyable
{
    xr_vector<Fvector> verts;
    xr_vector<TRI> faces;
    xr_vector<u32> flags;
    Fvector VMmin, VMscale;
    xr_vector<u32> VM[clpMX + 1][clpMY + 1][clpMZ + 1];
    Fvector VMeps;

    u32 VPack(const Fvector& V);

public:
    CollectorPacked(const Fbox& bb, u32 apx_vertices = 5000, u32 apx_faces = 5000);

    //		ICN CollectorPacked &operator=	(const CollectorPacked &object)
    //		{
    //			verts
    //		}

    void add_face(const Fvector& v0, const Fvector& v1, const Fvector& v2, u16 material, u16 sector, u32 flags);
    void add_face_D(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy, u32 flags);

    [[nodiscard]]
    auto getVS() const { return verts.size(); }
    [[nodiscard]]
    auto getTS() const { return faces.size(); }

    [[nodiscard]]
    auto& getV_Vec() const { return verts; }
    [[nodiscard]]
    auto& getV_Vec() { return verts; }

    [[nodiscard]]
    auto getV() const { return &verts.front(); }
    [[nodiscard]]
    auto getT() const { return &faces.front(); }

    [[nodiscard]]
    auto getV() { return &verts.front(); }
    [[nodiscard]]
    auto getT() { return &faces.front(); }

    [[nodiscard]]
    auto getfFlags(size_t index) const { return flags[index]; }
    [[nodiscard]]
    auto& getT(size_t index) { return faces[index]; }

    void clear();
};
#pragma warning(pop)
}
#pragma pack(pop)
