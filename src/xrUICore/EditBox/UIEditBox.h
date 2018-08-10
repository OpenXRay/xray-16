#pragma once
#include "xrUICore/Options/UIOptionsItem.h"
#include "xrUICore/EditBox/UICustomEdit.h"

class CUIFrameLineWnd;

class XRUICORE_API CUIEditBox : public CUIOptionsItem, public CUICustomEdit
{
public:
    CUIEditBox();

    virtual void InitCustomEdit(Fvector2 pos, Fvector2 size);

    // CUIOptionsItem
    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    // CUIMultiTextureOwner
    virtual void InitTexture(LPCSTR texture);
    virtual void InitTextureEx(LPCSTR texture, LPCSTR shader);

protected:
    CUIFrameLineWnd* m_frameLine;
    shared_str m_opt_backup_value;
};
