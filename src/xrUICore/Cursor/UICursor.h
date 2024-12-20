#pragma once

class CUIWindow;
class CUIStatic;

class XRUICORE_API CUICursor : public pureRender, public CDeviceResetNotifier, public CUIResetNotifier
{
    Fvector2 vPos{};
    CUIStatic* m_static{};
    Fvector2 vPrevPos{};
    Fvector2 correction;
    bool bVisible{};
    bool m_bound_to_system_cursor{};
    bool m_pause_autohide{};
    u32 m_become_visible_time{};

    void InitInternal();

public:
    CUICursor();
    ~CUICursor() override;

    void Show();
    void Hide();

    [[nodiscard]]
    bool IsVisible() const { return bVisible; }

    void OnRender() override;
    void OnDeviceReset() override;
    void OnUIReset() override;

    void UpdateAutohideTiming();
    void PauseAutohiding(bool pause);

    void WarpToWindow(CUIWindow* wnd, bool change_visibility = true);
    void UpdateCursorPosition(int _dx, int _dy);

    void SetUICursorPosition(Fvector2 pos);

    [[nodiscard]]
    Fvector2 GetCursorPosition() const;

    [[nodiscard]]
    Fvector2 GetCursorPositionDelta() const;
};
