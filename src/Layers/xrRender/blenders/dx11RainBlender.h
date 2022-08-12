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

class CBlender_rain_msaa final : public IBlender
{
    pcstr Name{};
    pcstr Definition{};

public:
    CBlender_rain_msaa() = default;
    CBlender_rain_msaa(pcstr name, pcstr definition)
        : Name(name), Definition(definition) {}

    virtual LPCSTR getComment() { return "INTERNAL: DX11 MSAA rain blender"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

#endif //	dx11RainBlender_included
