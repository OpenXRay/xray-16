#ifndef	dx11MSAABlender_included
#define	dx11MSAABlender_included

class CBlender_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: DX11 msaa blender"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

#endif	//	dx11RainBlender_included
