#ifndef	dx10MSAABlender_included
#define	dx10MSAABlender_included

class CBlender_msaa : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: DX10 msaa blender"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

#endif	//	dx10RainBlender_included
