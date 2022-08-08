#ifndef dx11MSAABlender_included
#define dx11MSAABlender_included

class CBlender_msaa : public IBlender
{
public:
    virtual LPCSTR getComment() { return "INTERNAL: DX11 msaa blender"; }
    virtual BOOL canBeDetailed() { return FALSE; }
    virtual BOOL canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

#endif //	dx11RainBlender_included
