#pragma once

class CBlender_bloom_build : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: combine to bloom target"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_bloom_build();
    virtual ~CBlender_bloom_build();
};

class CBlender_bloom_build_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: combine to bloom target msaa"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_bloom_build_msaa();
    virtual ~CBlender_bloom_build_msaa();
};

class CBlender_postprocess_msaa : public IBlender
{
public:
    LPCSTR getComment() override { return "INTERNAL: combine to bloom target msaa"; }
    BOOL canBeDetailed() override { return FALSE; }
    BOOL canBeLMAPped() override { return FALSE; }

    void Compile(CBlender_Compile& C) override;

    CBlender_postprocess_msaa();
    virtual ~CBlender_postprocess_msaa();
};
