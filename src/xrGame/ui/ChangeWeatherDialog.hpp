#pragma once

#include "UIDialogWnd.h"

class CUIStatic;
class CUITextWnd;
class CUI3tButton;
class CUIKickPlayer;
class CUIChangeMap;
class CUIXml;

class ButtonListDialog : public CUIDialogWnd
{
protected:
    struct NamedButton
    {
        CUI3tButton* Button;
        CUITextWnd* Text;
    };

    CUITextWnd* Header;
    CUIStatic* Background;
    CUI3tButton* CancelButton;

private:
    xr_vector<NamedButton> buttons;

protected:
    ButtonListDialog(pcstr window_name);
    void Initialize(int buttonCount);
    const NamedButton& GetButton(int i) const;
    // CUIDialogWnd
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboardAction) override;
    // ~CUIDialogWnd
    // CUIWindow
    virtual void SendMessage(CUIWindow* wnd, s16 msg, void* data = nullptr) override;
    // ~CUIWindow
    virtual void OnButtonClick(int i) {}
    virtual void OnCancel();

    pcstr GetDebugType() override { return "ButtonListDialog"; }
};

class ChangeWeatherDialog final : public ButtonListDialog
{
private:
    struct WeatherDesc
    {
        shared_str Name;
        shared_str Time;
    };

    xr_vector<WeatherDesc> weathers;

public:
    ChangeWeatherDialog() : ButtonListDialog(ChangeWeatherDialog::GetDebugType()) {}

    void InitChangeWeather(CUIXml& xmlDoc);
    // ButtonListDialog
    virtual void OnButtonClick(int i) override;
    // ~ButtonListDialog

    pcstr GetDebugType() override { return "ChangeWeatherDialog"; }

private:
    void ParseWeather();
};
// XXX nitrocaster: move to separate file
class ChangeGameTypeDialog final : public ButtonListDialog
{
private:
    xr_vector<shared_str> gameTypes;

public:
    ChangeGameTypeDialog() : ButtonListDialog(ChangeGameTypeDialog::GetDebugType()) {}

    void InitChangeGameType(CUIXml& xmlDoc);
    // ButtonListDialog
    virtual void OnButtonClick(int i) override;
    // ~ButtonListDialog

    pcstr GetDebugType() override { return "ChangeGameTypeDialog"; }
};
