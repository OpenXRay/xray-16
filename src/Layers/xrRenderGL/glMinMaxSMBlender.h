#ifndef	dx10MinMaxSMBlender_included
#define	dx10MinMaxSMBlender_included


class CBlender_createminmax : public IBlender
{
public:
    const char* getComment() override { return "INTERNAL: DX10 minmax sm blender"; }
    bool canBeDetailed() override { return FALSE; }
    bool canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

#endif
