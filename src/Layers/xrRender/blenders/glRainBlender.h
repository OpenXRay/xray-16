#ifndef	dx11RainBlender_included
#define	dx11RainBlender_included

class CBlender_rain : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: DX11 rain blender"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

class CBlender_rain_msaa : public IBlender
{
public:
    CBlender_rain_msaa()
    {
        Name = nullptr;
        Definition = nullptr;
    }

    LPCSTR getComment() override { return "INTERNAL: DX11 MSAA rain blender"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
    virtual void SetDefine(LPCSTR Name, LPCSTR Definition);

    LPCSTR Name;
    LPCSTR Definition;
};

#endif	//	dx11RainBlender_included
