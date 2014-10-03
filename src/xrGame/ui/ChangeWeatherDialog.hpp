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
    ButtonListDialog();
    virtual ~ButtonListDialog() {}
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
};

class ChangeWeatherDialog : public ButtonListDialog
{
private:
    struct WeatherDesc
    {
        shared_str Name;
        shared_str Time;
    };

    xr_vector<WeatherDesc> weathers;

public:
    void InitChangeWeather(CUIXml& xmlDoc);
    // ButtonListDialog
    virtual void OnButtonClick(int i) override;
    // ~ButtonListDialog

private:
    void ParseWeather();
};
// XXX nitrocaster: move to separate file
class ChangeGameTypeDialog : public ButtonListDialog
{
private:
    xr_vector<shared_str> gameTypes;

public:
    void InitChangeGameType(CUIXml& xmlDoc);
    // ButtonListDialog
    virtual void OnButtonClick(int i) override;
    // ~ButtonListDialog
};
