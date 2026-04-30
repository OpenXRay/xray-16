#pragma once

#if defined(USE_DX11)
#   define FVF_COLOR(c) ((c & 0xff00ff00) | ((c >> 16) & 0xff) | ((c & 0xff) << 16u))
#elif defined(USE_OGL)
#   define FVF_COLOR(c) (c)
#else
#   error No graphics API selected or enabled!
#endif

namespace xray::render::fg
{
namespace FVF
{
constexpr u32 RESERVED0          = 0x001;
constexpr u32 POSITION_MASK      = 0x400E;
constexpr u32 XYZ                = 0x002;
constexpr u32 XYZRHW             = 0x004;
constexpr u32 XYZB1              = 0x006;
constexpr u32 XYZB2              = 0x008;
constexpr u32 XYZB3              = 0x00a;
constexpr u32 XYZB4              = 0x00c;
constexpr u32 XYZB5              = 0x00e;
constexpr u32 XYZW               = 0x4002;
constexpr u32 NORMAL             = 0x010;
constexpr u32 PSIZE              = 0x020;
constexpr u32 DIFFUSE            = 0x040;
constexpr u32 SPECULAR           = 0x080;
constexpr u32 TEXCOUNT_MASK      = 0xf00;
constexpr u32 TEXCOUNT_SHIFT     = 8;
constexpr u32 TEX0               = 0x000;
constexpr u32 TEX1               = 0x100;
constexpr u32 TEX2               = 0x200;
constexpr u32 TEX3               = 0x300;
constexpr u32 TEX4               = 0x400;
constexpr u32 TEX5               = 0x500;
constexpr u32 TEX6               = 0x600;
constexpr u32 TEX7               = 0x700;
constexpr u32 TEX8               = 0x800;
constexpr u32 LASTBETA_UBYTE4    = 0x1000;
constexpr u32 LASTBETA_D3DCOLOR  = 0x8000;
constexpr u32 RESERVED2          = 0x6000;
constexpr u32 TEXTUREFORMAT2     = 0;
constexpr u32 TEXTUREFORMAT1     = 3;
constexpr u32 TEXTUREFORMAT3     = 1;
constexpr u32 TEXTUREFORMAT4     = 2;

constexpr u32 TEXCOORDSIZE3(u32 idx) { return 1u << (idx * 2 + 16); }

#pragma pack(push, 4)
struct L
{
    Fvector p;
    u32 color;
    void set(const L& src) { *this = src; };

    void set(float x, float y, float z, u32 C)
    {
        p.set(x, y, z);
        color = C;
    }

    void set(float x, float y, u32 C)
    {
        p.set(x, y, 1.0f);
        color = FVF_COLOR(C);
    }

    void set(const Fvector& _p, u32 C)
    {
        p.set(_p);
        color = C;
    }
};
const u32 F_L = XYZ | DIFFUSE;

struct V
{
    Fvector p;
    Fvector2 t;
    void set(const V& src) { *this = src; };

    void set(float x, float y, float z, float u, float v)
    {
        p.set(x, y, z);
        t.set(u, v);
    }

    void set(const Fvector& _p, float u, float v)
    {
        p.set(_p);
        t.set(u, v);
    }
};
const u32 F_V = XYZ | TEX1;

struct LIT
{
    Fvector p;
    u32 color;
    Fvector2 t;
    void set(const LIT& src) { *this = src; };

    void set(float x, float y, float z, u32 C, float u, float v)
    {
        p.set(x, y, z);
        color = C;
        t.set(u, v);
    }

    void set(const Fvector& _p, u32 C, float u, float v)
    {
        p.set(_p);
        color = C;
        t.set(u, v);
    }
};
const u32 F_LIT = XYZ | DIFFUSE | TEX1;

struct TL
{
    Fvector4 p;
    u32 color;
    Fvector2 uv;
    void set(const TL& src) { *this = src; };
    void set(float x, float y, u32 c, Fvector2& t) { set(x, y, .0001f, .9999f, c, t.x, t.y); };
    void set(float x, float y, u32 c, float u, float v) { set(x, y, .0001f, .9999f, c, u, v); };
    void set(int x, int y, u32 c, float u, float v) { set(float(x), float(y), .0001f, .9999f, c, u, v); };

    void set(float x, float y, float z, float w, u32 c, float u, float v)
    {
        p.set(x, y, z, w);
        color = c;
        uv.x = u;
        uv.y = v;
    };

    void transform(const Fvector& v, const Fmatrix& matSet)
    {
        // Transform it through the matrix set. Takes in mean projection.
        // Finally, scale the vertices to screen coords.
        // Note 1: device coords range from -1 to +1 in the viewport.
        // Note 2: the p.z-coordinate will be used in the z-buffer.
        p.w = matSet._14 * v.x + matSet._24 * v.y + matSet._34 * v.z + matSet._44;
        p.x = (matSet._11 * v.x + matSet._21 * v.y + matSet._31 * v.z + matSet._41) / p.w;
        p.y = -(matSet._12 * v.x + matSet._22 * v.y + matSet._32 * v.z + matSet._42) / p.w;
        p.z = (matSet._13 * v.x + matSet._23 * v.y + matSet._33 * v.z + matSet._43) / p.w;
    };
};
const u32 F_TL = XYZRHW | DIFFUSE | TEX1;

struct TL2uv
{
    Fvector4 p;
    u32 color;
    Fvector2 uv[2];
    void set(const TL2uv& src) { *this = src; };

    void set(float x, float y, u32 c, Fvector2& t0, Fvector2& t1)
    {
        set(x, y, .0001f, .9999f, c, t0.x, t0.y, t1.x, t1.y);
    };

    void set(float x, float y, float z, float w, u32 c, Fvector2& t0, Fvector2& t1)
    {
        set(x, y, z, w, c, t0.x, t0.y, t1.x, t1.y);
    };

    void set(float x, float y, u32 c, float u, float v, float u2, float v2)
    {
        set(x, y, .0001f, .9999f, c, u, v, u2, v2);
    };

    void set(int x, int y, u32 c, float u, float v, float u2, float v2)
    {
        set(float(x), float(y), .0001f, .9999f, c, u, v, u2, v2);
    };

    void set(float x, float y, float z, float w, u32 c, float u, float v, float u2, float v2)
    {
        p.set(x, y, z, w);
        color = c;
        uv[0].x = u;
        uv[0].y = v;
        uv[1].x = u2;
        uv[1].y = v2;
    };

    void transform(const Fvector& v, const Fmatrix& matSet)
    {
        // Transform it through the matrix set. Takes in mean projection.
        // Finally, scale the vertices to screen coords.
        // Note 1: device coords range from -1 to +1 in the viewport.
        // Note 2: the p.z-coordinate will be used in the z-buffer.
        p.w = matSet._14 * v.x + matSet._24 * v.y + matSet._34 * v.z + matSet._44;
        p.x = (matSet._11 * v.x + matSet._21 * v.y + matSet._31 * v.z + matSet._41) / p.w;
        p.y = -(matSet._12 * v.x + matSet._22 * v.y + matSet._32 * v.z + matSet._42) / p.w;
        p.z = (matSet._13 * v.x + matSet._23 * v.y + matSet._33 * v.z + matSet._43) / p.w;
    };
};
const u32 F_TL2uv = XYZRHW | DIFFUSE | TEX2;

struct TL4uv
{
    Fvector4 p;
    u32 color;
    Fvector2 uv[4];
    void set(const TL4uv& src) { *this = src; };

    void set(float x, float y, u32 c, Fvector2& t0, Fvector2& t1)
    {
        set(x, y, .0001f, .9999f, c, t0.x, t0.y, t1.x, t1.y);
    };

    void set(float x, float y, float z, float w, u32 c, Fvector2& t0, Fvector2& t1)
    {
        set(x, y, z, w, c, t0.x, t0.y, t1.x, t1.y);
    };

    void set(float x, float y, u32 c, float u, float v, float u2, float v2)
    {
        set(x, y, .0001f, .9999f, c, u, v, u2, v2);
    };

    void set(int x, int y, u32 c, float u, float v, float u2, float v2)
    {
        set(float(x), float(y), .0001f, .9999f, c, u, v, u2, v2);
    };

    void set(float x, float y, float z, float w, u32 c, float u, float v, float u2, float v2)
    {
        p.set(x, y, z, w);
        color = c;
        uv[0].x = u;
        uv[0].y = v;
        uv[1].x = u2;
        uv[1].y = v2;
    };
};
const u32 F_TL4uv = XYZRHW | DIFFUSE | TEX4;
#pragma pack(pop)
} // namespace FVF
} // namespace xray::render::fg
