#pragma once

class CBlender_dof : public IBlender
{
public:
    CBlender_dof();
    ~CBlender_dof() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};
