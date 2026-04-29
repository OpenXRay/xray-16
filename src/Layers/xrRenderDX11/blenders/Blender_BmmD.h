#pragma once

namespace xray::render::RENDER_NAMESPACE
{
class CBlender_BmmD : public IBlender
{
    string64 oT2_Name;
    string64 oT2_xform;
    string64 oR_Name;
    string64 oG_Name;
    string64 oB_Name;
    string64 oA_Name;

private:
    void CompileFFP(CBlender_Compile& C) const;
    void CompileProgrammable(CBlender_Compile& C) const;

public:
    LPCSTR getComment() override;
    BOOL canBeDetailed() override;
    BOOL canBeLMAPped() override;
    BOOL canUseSteepParallax() override;
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;

    void Compile(CBlender_Compile& C) override;

    const char* GetDetailR() const { return oR_Name; }
    const char* GetDetailG() const { return oG_Name; }
    const char* GetDetailB() const { return oB_Name; }
    const char* GetDetailA() const { return oA_Name; }

    CBlender_BmmD();
    ~CBlender_BmmD() override = default;
};
}
