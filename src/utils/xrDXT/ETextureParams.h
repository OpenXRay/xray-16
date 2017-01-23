#ifndef ETextureParamsH
#define ETextureParamsH

#pragma pack(push,1)
struct ECORE_API STextureParams
{
    enum ETType
    {
        ttImage = 0,
        ttCubeMap,
        ttBumpMap,
        ttNormalMap,
        ttTerrain,
        ttForceU32 = u32(-1)
    };

    enum ETFormat
    {
        tfDXT1 = 0,
        tfADXT1,
        tfDXT3,
        tfDXT5,
        tf4444,
        tf1555,
        tf565,
        tfRGB,
        tfRGBA,
        tfNVHS,
        tfNVHU,
        tfA8,
        tfL8,
        tfA8L8,
        tfForceU32 = u32(-1)
    };

    enum ETBumpMode
    {
        tbmResereved = 0,
        tbmNone,
        tbmUse,
        tbmForceU32 = u32(-1)
    };

    enum ETMaterial
    {
        tmOrenNayar_Blin = 0,
        tmBlin_Phong,
        tmPhong_Metal,
        tmMetal_OrenNayar,
        tmForceU32 = u32(-1)
    };
    
    enum
    {
        kMIPFilterAdvanced = 5,
        kMIPFilterPoint = 2,
        kMIPFilterBox = 0,
        kMIPFilterTriangle = 3,
        kMIPFilterQuadratic = 4,
        kMIPFilterCubic = 1,
        kMIPFilterCatrom = 6,
        kMIPFilterMitchell = 7,
        kMIPFilterGaussian = 8,
        kMIPFilterSinc = 9,
        kMIPFilterBessel = 10,
        kMIPFilterHanning = 11,
        kMIPFilterHamming = 12,
        kMIPFilterBlackman = 13,
        kMIPFilterKaiser = 14,
    };

    enum
    {
        flGenerateMipMaps = (1 << 0),
        flBinaryAlpha = (1 << 1),
        flAlphaBorder = (1 << 4),
        flColorBorder = (1 << 5),
        flFadeToColor = (1 << 6),
        flFadeToAlpha = (1 << 7),
        flDitherColor = (1 << 8),
        flDitherEachMIPLevel = (1 << 9),
        flDiffuseDetail = (1 << 23),
        flImplicitLighted = (1 << 24),
        flHasAlpha = (1 << 25),
        flBumpDetail = (1 << 26),
        flForceU32 = u32(-1)
    };

    // texture part
    ETFormat fmt;
    Flags32 flags;
    u32 border_color;
    u32 fade_color;
    u32 fade_amount;
    u8 fade_delay;
    u32 mip_filter;
    int width;
    int height;
    // detail ext
    shared_str detail_name;
    float detail_scale;
    ETType type;
    // material
    ETMaterial material;
    float material_weight;
    // bump	
    float bump_virtual_height;
    ETBumpMode bump_mode;
    shared_str bump_name;
    shared_str ext_normal_map_name;

    STextureParams()
    {
        ZeroMemory(this, sizeof(STextureParams));
        flags.set(flGenerateMipMaps | flDitherColor, TRUE);
        mip_filter = kMIPFilterBox;
        width = 0;
        height = 0;
        detail_scale = 1;
        bump_mode = tbmNone;
        material = tmBlin_Phong;
        bump_virtual_height = 0.05f;
    }

    IC BOOL HasAlpha()
    {
        // исходная текстура содержит альфа канал
        return flags.is(flHasAlpha);
    }
    
    IC BOOL HasAlphaChannel() // игровая текстура содержит альфа канал
    {
        switch (fmt)
        {
        case tfADXT1:
        case tfDXT3:
        case tfDXT5:
        case tf4444:
        case tf1555:
        case tfRGBA:
            return TRUE;
        default:
            return FALSE;
        }
    }
    void Load(IReader& F);
    void Save(IWriter& F);
#ifdef _EDITOR
    PropValue::TOnChange OnTypeChangeEvent;
    void __stdcall OnTypeChange(PropValue* v);
    void FillProp(LPCSTR base_name, PropItemVec& items, PropValue::TOnChange OnChangeEvent);
    LPCSTR FormatString();
    u32 MemoryUsage(LPCSTR base_name);
#endif
};
#pragma pack(pop)

struct xr_token;
extern xr_token	tparam_token[];
extern xr_token	tfmt_token[];
extern xr_token	ttype_token[];

#define THM_CHUNK_VERSION 0x0810
#define THM_CHUNK_DATA 0x0811
#define THM_CHUNK_TEXTUREPARAM 0x0812
#define THM_CHUNK_TYPE 0x0813
#define THM_CHUNK_TEXTURE_TYPE 0x0814
#define THM_CHUNK_DETAIL_EXT 0x0815
#define THM_CHUNK_MATERIAL 0x0816
#define THM_CHUNK_BUMP 0x0817
#define THM_CHUNK_EXT_NORMALMAP 0x0818
#define THM_CHUNK_FADE_DELAY 0x0819
#define THUMB_WIDTH 128
#define THUMB_HEIGHT 128
#define THUMB_SIZE THUMB_HEIGHT*THUMB_WIDTH

#endif /*_INCDEF_TextureParams_H_*/
