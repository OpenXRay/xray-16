#pragma once

#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Options/UIOptionsItem.h"

struct game_action;
struct keyboard_key;
class CUIColorAnimatorWrapper;

class CUIEditKeyBind final : public CUIStatic, public CUIOptionsItem
{
    bool m_primary;
    bool m_isGamepadBinds;
    game_action* m_action;
    keyboard_key* m_keyboard;
    keyboard_key* m_opt_backup_value;

public:
    CUIEditKeyBind(bool primary, bool isGamepadBinds = false);

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
    bool OnControllerAction(int axis, float x, float y, EUIMessages controller_action) override;

    virtual void SetText(LPCSTR text);
    void SetEditMode(bool b);

    pcstr GetDebugType() override { return "CUIEditKeyBind"; }

protected:
    void BindAction2Key();

    bool m_isEditMode;

    //.	CUIColorAnimatorWrapper*				m_pAnimation;
};
