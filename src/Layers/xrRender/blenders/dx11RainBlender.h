#ifndef dx11RainBlender_included
#define dx11RainBlender_included

class CBlender_rain : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: DX11 rain blender"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

class CBlender_rain_msaa : public IBlender
{
public:
    CBlender_rain_msaa()
    {
        Name = 0;
        Definition = 0;
    }
    virtual LPCSTR getComment() { return "INTERNAL: DX11 MSAA rain blender"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
    virtual void SetDefine(LPCSTR Name, LPCSTR Definition);

    LPCSTR Name;
    LPCSTR Definition;
};

#endif //	dx11RainBlender_included
