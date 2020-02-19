#ifndef dx11MinMaxSMBlender_included
#define dx11MinMaxSMBlender_included

class CBlender_createminmax : public IBlender
{
public:
    virtual const char* getComment() { return "INTERNAL: DX10 minmax sm blender"; }
    virtual bool canBeDetailed() { return FALSE; }
    virtual bool canBeLMAPped() { return FALSE; }
    virtual void Compile(CBlender_Compile& C);
};

#endif
