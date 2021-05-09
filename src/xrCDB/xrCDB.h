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
template <class T> class _box3;
using Fbox = _box3<float>;
class Lock;


#pragma pack(push, 8)
namespace CDB
{
// Triangle
class XRCDB_API TRI //*** 16 bytes total (was 32 :)
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
    IC u32 IDvert(u32 ID) { return verts[ID]; }
};

static_assert(sizeof(TRI) == 16, "TRI always should be 16 bytes on any architecture.");

// Build callback
typedef void __stdcall build_callback(Fvector* V, int Vcnt, TRI* T, int Tcnt, void* params);

// Model definition
class XRCDB_API MODEL : Noncopyable
{
    friend class COLLIDER;
    enum
    {
        S_READY = 0,
        S_INIT = 1,
        S_BUILD = 2,
        S_forcedword = u32(-1)
    };

private:
    Lock* pcs;
    Opcode::OPCODE_Model* tree;
    volatile u32 status; // 0=ready, 1=init, 2=building
    u32 version;

    // tris
    TRI* tris;
    int tris_count;
    Fvector* verts;
    int verts_count;

public:
    MODEL();
    ~MODEL();

    IC Fvector* get_verts() { return verts; }
    IC const Fvector* get_verts() const { return verts; }
    IC int get_verts_count() const { return verts_count; }
    IC const TRI* get_tris() const { return tris; }
    IC TRI* get_tris() { return tris; }
    IC int get_tris_count() const { return tris_count; }
    IC void syncronize() const
    {
        if (S_READY != status)
            syncronize_impl();
    }

    static void build_thread(void*);
    void build_internal(Fvector* V, int Vcnt, TRI* T, int Tcnt, build_callback* bc = NULL, void* bcp = NULL);
    void build(Fvector* V, int Vcnt, TRI* T, int Tcnt, build_callback* bc = NULL, void* bcp = NULL);
    u32 memory();

    void set_version(u32 value) { version = value; }
    bool serialize(pcstr fileName) const;
    bool deserialize(pcstr fileName);

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
    // Ray data and methods
    u32 ray_mode;
    u32 box_mode;
    u32 frustum_mode;

    // Result management
    xr_vector<RESULT> rd;

public:
    COLLIDER();
    ~COLLIDER();

    ICF void ray_options(u32 f) { ray_mode = f; }
    void ray_query(const MODEL* m_def, const Fvector& r_start, const Fvector& r_dir, float r_range = 10000.f);

    ICF void box_options(u32 f) { box_mode = f; }
    void box_query(const MODEL* m_def, const Fvector& b_center, const Fvector& b_dim);

    ICF void frustum_options(u32 f) { frustum_mode = f; }
    void frustum_query(const MODEL* m_def, const CFrustum& F);

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
    void calc_adjacency(xr_vector<u32>& dest);

    Fvector* getV() { return &*verts.begin(); }
    size_t getVS() { return verts.size(); }
    TRI* getT() { return &*faces.begin(); }
    size_t getTS() { return faces.size(); }
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
    using DWORDList = xr_vector<u32>;
    using DWORDIt = DWORDList::iterator;

private:
    xr_vector<Fvector> verts;
    xr_vector<TRI> faces;
    xr_vector<u32> flags;
    Fvector VMmin, VMscale;
    DWORDList VM[clpMX + 1][clpMY + 1][clpMZ + 1];
    Fvector VMeps;

    u32 VPack(const Fvector& V);

public:
    CollectorPacked(const Fbox& bb, int apx_vertices = 5000, int apx_faces = 5000);

    //		__declspec(noinline) CollectorPacked &operator=	(const CollectorPacked &object)
    //		{
    //			verts
    //		}

    void add_face(const Fvector& v0, const Fvector& v1, const Fvector& v2, u16 material, u16 sector, u32 flags);
    void add_face_D(const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy, u32 flags);

    xr_vector<Fvector>& getV_Vec() { return verts; }
    Fvector* getV() { return &verts.front(); }
    size_t getVS() { return verts.size(); }
    TRI* getT() { return &faces.front(); }
    u32 getfFlags(size_t index) { return flags[index]; }
    IC TRI& getT(size_t index) { return faces[index]; }
    size_t getTS() { return faces.size(); }
    void clear();
};
#pragma warning(pop)
}
#pragma pack(pop)
