#pragma once
#include "xrUICore/EditBox/UICustomEdit.h"

class CUIFrameWindow;

class CUIEditBoxEx : public CUICustomEdit
{
public:
    CUIEditBoxEx();
    virtual ~CUIEditBoxEx();

    virtual void InitCustomEdit(Fvector2 pos, Fvector2 size);

    // CUIMultiTextureOwner
    virtual void InitTexture(LPCSTR texture);
    virtual void InitTextureEx(LPCSTR texture, LPCSTR shader);

protected:
    CUIFrameWindow* m_pFrameWindow;
};
