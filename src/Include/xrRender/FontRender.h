#ifndef FontRender_included
#define FontRender_included
#pragma once

class CGameFont;

class IFontRender
{
public:
    virtual ~IFontRender() {}
    virtual void Initialize(cpcstr cShader, cpcstr cTexture) = 0;
    virtual void OnRender(CGameFont& owner) = 0;
};

#endif //	FontRender_included
