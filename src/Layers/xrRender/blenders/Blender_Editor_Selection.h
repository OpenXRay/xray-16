#pragma once

class CBlender_Editor_Selection : public IBlender
{
    string64 oT_Factor;
public:
    CBlender_Editor_Selection();
    ~CBlender_Editor_Selection() override = default;

    LPCSTR getComment() override;
    BOOL canBeLMAPped() override;
    
    void Save(IWriter& fs) override;
    void Load(IReader& fs, u16 version) override;
    
    void Compile(CBlender_Compile& C) override;
};
