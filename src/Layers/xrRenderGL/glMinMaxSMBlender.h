#ifndef	dx10MinMaxSMBlender_included
#define	dx10MinMaxSMBlender_included


class CBlender_createminmax : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: DX10 minmax sm blender"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;
};

#endif
