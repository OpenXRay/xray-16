#pragma once

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Options/UIOptionsItem.h"

struct _action;
struct _keyboard;
class CUIColorAnimatorWrapper;

class CUIEditKeyBind : public CUIStatic, public CUIOptionsItem
{
    bool m_bPrimary;
    _action* m_action;
    _keyboard* m_keyboard;
    _keyboard* m_opt_backup_value;

public:
    CUIEditKeyBind(bool bPrim);
    virtual ~CUIEditKeyBind();
    // options item
    virtual void AssignProps(const shared_str& entry, const shared_str& group);

    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    void SetValue();
    virtual void OnMessage(LPCSTR message);

    // CUIWindow methods
    void InitKeyBind(Fvector2 pos, Fvector2 size);
    virtual void Update();
    virtual bool OnMouseDown(int mouse_btn);
    virtual void OnFocusLost();
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);

    virtual void SetText(LPCSTR text);
    void SetEditMode(bool b);

protected:
    void BindAction2Key();

    bool m_bIsEditMode;

    //.	CUIColorAnimatorWrapper*				m_pAnimation;
};
