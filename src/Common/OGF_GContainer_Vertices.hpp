#pragma once

inline u32 u8_vec4(Fvector N, u8 A = 0)
{
    N.add(1.f);
    N.mul(.5f * 255.f);
    s32 nx = iFloor(N.x);
    clamp(nx, 0, 255);
    s32 ny = iFloor(N.y);
    clamp(ny, 0, 255);
    s32 nz = iFloor(N.z);
    clamp(nz, 0, 255);
    return color_rgba(nx, ny, nz, A);
}

#ifdef LEVEL_COMPILER
inline u32 u8_vec4(base_basis N, u8 A = 0)
{
    return color_rgba(N.x, N.y, N.z, A);
}
#endif

inline std::pair<s16, u8> s24_tc_base(float uv) // [-32 .. +32]
{
    const u32 max_tile = 32;
    const s32 quant = 32768 / max_tile;

    float rebased = uv * float(quant);
    s32 _primary = iFloor(rebased);
    clamp(_primary, -32768, 32767);
    s32 _secondary = iFloor(255.5f * (rebased - float(_primary)));
    clamp(_secondary, 0, 255);
    return std::make_pair(s16(_primary), u8(_secondary));
}

inline s16 s16_tc_lmap(float uv) // [-1 .. +1]
{
    const u32 max_tile = 1;
    const s32 quant = 32768 / max_tile;

    s32 t = iFloor(uv * float(quant));
    clamp(t, -32768, 32767);
    return s16(t);
}

constexpr D3DVERTEXELEMENT9 r1_decl_lmap[] = // 12+4+4+4+4+4 = 32
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
    { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
    { 0, 24, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    { 0, 28, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
    D3DDECL_END()
};

constexpr D3DVERTEXELEMENT9 r1_decl_lmap_unpacked[] = // 12+4+8+8 = 32
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
    { 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
    D3DDECL_END()
};

constexpr D3DVERTEXELEMENT9 r1_decl_vert[] = // 12+4+4+4+4+4 = 32
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
    { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
    { 0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    { 0, 28, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};

constexpr D3DVERTEXELEMENT9 r1_decl_vert_unpacked[] = // 12+4+4+8 = 28
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    { 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};

constexpr D3DVERTEXELEMENT9 x_decl_vert[] = // 12
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    D3DDECL_END()
};

constexpr D3DVERTEXELEMENT9 mu_model_decl[] = // 12+4+4+4+8 = 32
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
    { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
    { 0, 24, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};

constexpr D3DVERTEXELEMENT9 mu_model_decl_unpacked[] = // 12+4+4+8 = 28
{
    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    { 0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};
#pragma pack(push, 1)
struct x_vert
{
    Fvector3 P;
    x_vert(Fvector3 _P) { P = _P; }
};

struct r1v_lmap
{
    Fvector3 P;
    u32 N;
    u32 T;
    u32 B;
    _vector2<s16> tc0;
    _vector2<s16> tc1;

#ifdef LEVEL_COMPILER
    r1v_lmap(Fvector3 _P, Fvector _N, base_basis _T, base_basis _B, base_color _CC, Fvector2 tc_base, Fvector2 tc_lmap)
    {
        base_color_c _C;
        _CC._get(_C);
        _N.normalize();
        std::pair<s16, u8> tc_u = s24_tc_base(tc_base.x);
        std::pair<s16, u8> tc_v = s24_tc_base(tc_base.y);
        P = _P;
        N = u8_vec4(_N, u8_clr(_C.hemi));
        T = u8_vec4(_T, tc_u.second);
        B = u8_vec4(_B, tc_v.second);
        tc0.x = tc_u.first;
        tc0.y = tc_v.first;
        tc1.x = s16_tc_lmap(tc_lmap.x);
        tc1.y = s16_tc_lmap(tc_lmap.y);
    }
#endif // LEVEL_COMPILER
};

struct r1v_lmap_unpacked
{
    Fvector3 P;
    u32 N;
    Fvector2 tc0;
    Fvector2 tc1;

    r1v_lmap_unpacked& operator=(const r1v_lmap& packed)
    {
        P = packed.P;

        Fcolor unpackedN(packed.N);
        unpackedN.mul_rgb(2);
        unpackedN.sub_rgb(1);
        N = unpackedN.get();

        Fcolor T(packed.T);
        Fcolor B(packed.B);

        tc0.x = (packed.tc0.x + T.a) * (32.f / 32768.f);
        tc0.y = (packed.tc0.y + B.a) * (32.f / 32768.f);
        tc1.x = packed.tc1.x * (1.f / 32768.f);
        tc1.y = packed.tc1.y * (1.f / 32768.f);

        return *this;
    }

};

struct r1v_vert
{
    Fvector3 P;
    u32 N;
    u32 T;
    u32 B;
    u32 C;
    _vector2<s16> tc;

#ifdef LEVEL_COMPILER
    r1v_vert(Fvector3 _P, Fvector _N, base_basis _T, base_basis _B, base_color _CC, Fvector2 tc_base)
    {
        base_color_c _C;
        _CC._get(_C);
        _N.normalize();
        std::pair<s16, u8> tc_u = s24_tc_base(tc_base.x);
        std::pair<s16, u8> tc_v = s24_tc_base(tc_base.y);
        P = _P;
        N = u8_vec4(_N, u8_clr(_C.hemi));
        T = u8_vec4(_T, tc_u.second);
        B = u8_vec4(_B, tc_v.second);
        C = color_rgba(u8_clr(_C.rgb.x), u8_clr(_C.rgb.y), u8_clr(_C.rgb.z), u8_clr(_C.sun));
        tc.x = tc_u.first;
        tc.y = tc_v.first;
    }
#endif // XRLC_LIGHT_EXPORTS
};

struct r1v_vert_unpacked
{
    Fvector3 P;
    u32 N;
    u32 C;
    Fvector2 tc;

    r1v_vert_unpacked& operator=(const r1v_vert& packed)
    {
        P = packed.P;

        Fcolor unpackedN(packed.N);
        unpackedN.mul_rgb(2);
        unpackedN.sub_rgb(1);
        N = unpackedN.get();

        C = packed.C;

        Fcolor T(packed.T);
        Fcolor B(packed.B);

        tc.x = (packed.tc.x + T.a) * (32.f / 32768.f);
        tc.y = (packed.tc.y + B.a) * (32.f / 32768.f);
        return *this;
    }
};

struct mu_model_vert
{
    Fvector3 P;
    u32 N;
    u32 T;
    u32 B;
    _vector4<s16> misc;
};

struct mu_model_vert_unpacked
{
    Fvector3 P;
    u32 N;
    u32 C;
    Fvector2 tc;

    mu_model_vert_unpacked& operator=(const mu_model_vert& packed)
    {
        P = packed.P;

        Fcolor unpackedN(packed.N);
        unpackedN.mul_rgb(2);
        unpackedN.sub_rgb(1);
        N = unpackedN.get();

        tc.x = (packed.misc.x) * (32.f / 32768.f);
        tc.y = (packed.misc.y) * (32.f / 32768.f);

        return *this;
    }
};

#pragma pack(pop)
