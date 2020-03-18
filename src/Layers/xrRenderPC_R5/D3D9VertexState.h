
REGISTER(D3DFVF_RESERVED0       ,0x001, 0,"", VF_NONE)
REGISTER(D3DFVF_POSITION_MASK   ,0x400E, 0,"", VF_NONE)
REGISTER(D3DFVF_XYZ             ,0x002, sizeof(float)*3,"position_xyz", VF_R32G32B32_FLOAT)
REGISTER(D3DFVF_XYZRHW          ,0x004, sizeof(float)*4,"position_xyzrhw", VF_R32G32B32A32_FLOAT)
REGISTER(D3DFVF_XYZB1           ,0x006, 0, "", VF_NONE)
REGISTER(D3DFVF_XYZB2           ,0x008, 0, "", VF_NONE)
REGISTER(D3DFVF_XYZB3           ,0x00a, 0, "", VF_NONE)
REGISTER(D3DFVF_XYZB4           ,0x00c, 0, "", VF_NONE)
REGISTER(D3DFVF_XYZB5           ,0x00e, 0, "", VF_NONE)
REGISTER(D3DFVF_XYZW            ,0x4002, sizeof(float)*4, "position_xyzw", VF_R32G32B32A32_FLOAT)

REGISTER(D3DFVF_NORMAL          ,0x010, sizeof(float), "", VF_NONE)
REGISTER(D3DFVF_PSIZE           ,0x020, sizeof(float), "", VF_NONE)
REGISTER(D3DFVF_DIFFUSE         ,0x040, sizeof(int32), "diffuse", VF_R8G8B8A8)
REGISTER(D3DFVF_SPECULAR        ,0x080, sizeof(int32), "specular", VF_R8G8B8A8)

REGISTER(D3DFVF_TEXCOUNT_MASK   ,0xf00, 0, "", VF_NONE)
REGISTER(D3DFVF_TEXCOUNT_SHIFT   ,8, 0, "", VF_NONE)
//REGISTER(D3DFVF_TEX0            ,0x000, sizeof(float)*2, "uv0", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX1            ,0x100, sizeof(float)*2, "uv1t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX2            ,0x200, sizeof(float)*2, "uv2t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX3            ,0x300, sizeof(float) * 2, "uv3t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX4            ,0x400, sizeof(float) * 2, "uv4t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX5            ,0x500, sizeof(float) * 2, "uv5t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX6            ,0x600, sizeof(float) * 2, "uv6t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX7            ,0x700, sizeof(float) * 2, "uv7t", VF_R32G32_FLOAT)
REGISTER(D3DFVF_TEX8            ,0x800, sizeof(float) * 2, "uv8t", VF_R32G32_FLOAT)

REGISTER(D3DFVF_LASTBETA_UBYTE4  ,0x1000, 0, "", VF_NONE)
REGISTER(D3DFVF_LASTBETA_D3DCOLOR,0x8000, 0, "", VF_NONE)

REGISTER(D3DFVF_RESERVED2        ,0x6000, 0, "", VF_NONE) // 2 reserved bits
REGISTER(D3DFVF_COUNT, 28, 0, "", VF_NONE) // 2 reserved bits