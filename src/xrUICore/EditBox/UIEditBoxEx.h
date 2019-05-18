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
    virtual bool InitTexture(pcstr texture, bool fatal = true);
    virtual bool InitTextureEx(pcstr texture, pcstr shader, bool fatal = true);

protected:
    CUIFrameWindow* m_pFrameWindow;
};
