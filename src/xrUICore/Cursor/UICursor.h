#pragma once

class CUIStatic;

class XRUICORE_API CUICursor : public pureRender, public CUIResetAndResolutionNotifier
{
    bool bVisible;
    Fvector2 vPos;
    Fvector2 vPrevPos;
    bool m_b_use_win_cursor;
    CUIStatic* m_static;
    void InitInternal();

public:
    CUICursor();
    virtual ~CUICursor();
    virtual void OnRender();

    Fvector2 GetCursorPositionDelta();

    Fvector2 GetCursorPosition();
    void SetUICursorPosition(Fvector2 pos);
    void UpdateCursorPosition(int _dx, int _dy);

    void OnUIReset() override;

    bool IsVisible() { return bVisible; }
    void Show();
    void Hide();
};
