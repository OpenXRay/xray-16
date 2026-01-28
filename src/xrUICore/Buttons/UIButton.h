#pragma once

#include "xrUICore/Static/UIStatic.h"

class XRUICORE_API CUIButton : public CUIStatic
{
protected:
    using inherited = CUIStatic;

public:
    CUIButton();
    ~CUIButton() override;

    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnClick();

    //прорисовка окна
    virtual void DrawTexture();
    virtual void DrawText();

    virtual void Update();
    virtual void Enable(bool status);
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual void OnFocusLost();

    //состояния в которых находится кнопка
    enum E_BUTTON_STATE : u8
    {
        BUTTON_NORMAL, //кнопка никак не затрагивается
        BUTTON_PUSHED, //в нажатом сотоянии
        BUTTON_UP //при удерживаемой кнопки мыши
    };

    //заново подготовить состояние
    virtual void Reset();

    // Установка состояния кнопки: утоплена, не утоплена
    void SetButtonState(E_BUTTON_STATE eBtnState) { m_eButtonState = eBtnState; }
    E_BUTTON_STATE GetButtonState() const { return m_eButtonState; }
    // Поведение кнопки как переключателя реализовано пока только в режиме NORMAL_PRESS
    void SetButtonAsSwitch(bool bAsSwitch) { m_bIsSwitch = bAsSwitch; }
    // Работа с акселератором
    // Код акселератора берётся из файла SDL_scancode.h, из SDL.
    // Например: кнопка A - код 4 (SDL_SCANCODE_A)
    void SetAccelerator(int iAccel, bool isKey, size_t idx);
    int GetAccelerator(size_t idx) const;
    bool IsAccelerator(int iAccel) const;

    shared_str m_hint_text;

    pcstr GetDebugType() override { return "CUIButton"; }

protected:
    struct alignas(2) ButtonAccelerator
    {
        s16  accel : 15;
        bool isKey : 1;
    };

    ButtonAccelerator m_accelerators[4]{};
    E_BUTTON_STATE m_eButtonState;
    bool m_bIsSwitch;

private:
    DECLARE_SCRIPT_REGISTER_FUNCTION(CUIStatic, CUIWindow);
};
