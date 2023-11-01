#pragma once

class CUIWindow;
class CUIStatic;

class XRUICORE_API CUICursor : public pureRender, public CDeviceResetNotifier, public CUIResetNotifier
{
    bool bVisible{};
    Fvector2 vPos{};
    Fvector2 vPrevPos{};
    Fvector2 correction;
    bool m_bound_to_system_cursor{};
    CUIStatic* m_static{};
    u32 m_become_visible_time{};
    bool m_pause_autohide{};

    void InitInternal();

public:
    CUICursor();
    virtual ~CUICursor();
    virtual void OnRender();

    void UpdateAutohideTiming();
    void PauseAutohiding(bool pause);

    Fvector2 GetCursorPositionDelta();

    Fvector2 GetCursorPosition();
    void SetUICursorPosition(Fvector2 pos);
    void WarpToWindow(CUIWindow* wnd, bool change_visibility = true);
    void UpdateCursorPosition(int _dx, int _dy);

    void OnDeviceReset() override;
    void OnUIReset() override;

    bool IsVisible() const { return bVisible; }
    void Show();
    void Hide();
};
