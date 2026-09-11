#ifndef UIShader_included
#define UIShader_included
#pragma once

class IUIShader
{
public:
    virtual ~IUIShader() { ; }
    virtual void Copy(IUIShader& _in) = 0;
    virtual void create(LPCSTR sh, LPCSTR tex = nullptr) = 0;
    virtual bool inited() = 0;
    virtual void destroy() = 0;

    [[nodiscard]]
    virtual bool operator==(const IUIShader& other) const = 0;

    virtual bool GetBaseTextureResolution(Fvector2& res) = 0;
    virtual xrImTextureData GetImGuiTextureId() = 0;
};

#endif //	UIShader_included
