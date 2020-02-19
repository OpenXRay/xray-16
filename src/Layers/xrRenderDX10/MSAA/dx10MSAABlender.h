#ifndef dx10MSAABlender_included
#define dx10MSAABlender_included

class CBlender_msaa : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: DX10 msaa blender"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

#endif //	dx10RainBlender_included
