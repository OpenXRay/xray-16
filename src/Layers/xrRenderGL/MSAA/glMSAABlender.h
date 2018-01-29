#ifndef	dx10MSAABlender_included
#define	dx10MSAABlender_included

class CBlender_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: DX10 msaa blender"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

#endif	//	dx10RainBlender_included
