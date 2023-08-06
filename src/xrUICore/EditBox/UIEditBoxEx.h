#pragma once
#include "xrUICore/EditBox/UICustomEdit.h"

class CUIFrameWindow;

class CUIEditBoxEx final : public CUICustomEdit
{
public:
    CUIEditBoxEx();
    ~CUIEditBoxEx() override;

    virtual void InitCustomEdit(Fvector2 pos, Fvector2 size);

    // CUIMultiTextureOwner
    virtual bool InitTexture(pcstr texture, bool fatal = true);
    virtual bool InitTextureEx(pcstr texture, pcstr shader, bool fatal = true);

    pcstr GetDebugType() override { return "CUIEditBoxEx"; }

protected:
    CUIFrameWindow* m_pFrameWindow;
};
