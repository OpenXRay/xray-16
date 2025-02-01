#pragma once

class CBlender_Blur : public IBlender
{
public:
    CBlender_Blur();
    ~CBlender_Blur() override = default;

    LPCSTR getComment() override;
    void Compile(CBlender_Compile& C) override;
};

// SSS
class CBlender_ssfx_ssr : public IBlender
{
public:
    CBlender_ssfx_ssr();
    ~CBlender_ssfx_ssr() override = default;

    LPCSTR getComment() override { return "ssfx_ssr"; }
    void Compile(CBlender_Compile& C) override;
};

class CBlender_ssfx_volumetric_blur : public IBlender
{
public:
    CBlender_ssfx_volumetric_blur();
    ~CBlender_ssfx_volumetric_blur() override = default;

    LPCSTR getComment() override { return "ssfx_volumetric_blur"; }
    void Compile(CBlender_Compile& C) override;
};

class CBlender_ssfx_ao : public IBlender
{
public:
    CBlender_ssfx_ao();
    ~CBlender_ssfx_ao() override = default;

    LPCSTR getComment() override { return "ssfx_ao"; }
    void Compile(CBlender_Compile& C) override;
};
