#pragma once
#include "xrUICore/Options/UIOptionsItem.h"
#include "xrUICore/InteractiveBackground/UI_IB_Static.h"

class CUI3tButton;

class XRUICORE_API CUITrackBar : public CUI_IB_FrameLineWnd, public CUIOptionsItem
{
public:
    CUITrackBar();
    // CUIOptionsItem
    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    virtual void Draw();
    virtual void Update();
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);
    virtual void OnMessage(LPCSTR message);
    // CUIWindow
    void InitTrackBar(Fvector2 pos, Fvector2 size);
    virtual void Enable(bool status);
    void SetInvert(bool v) { m_b_invert = v; }
    bool GetInvert() const { return m_b_invert; };
    void SetStep(float step);
    void SetType(bool b_float) { m_b_is_float = b_float; };
    void SetBoundReady(bool b_ready) { m_b_bound_already_set = b_ready; };
    bool GetCheck();
    void SetCheck(bool b);
    int GetIValue() { return m_i_val; }
    float GetFValue() { return m_f_val; }
    void SetOptIBounds(int imin, int imax);
    void SetOptFBounds(float fmin, float fmax);

    CUIStatic* m_static;
    shared_str m_static_format;

protected:
    void UpdatePos();
    void UpdatePosRelativeToMouse();

    CUI3tButton* m_pSlider;
    bool m_b_invert;
    bool m_b_is_float;
    bool m_b_mouse_capturer;
    bool m_b_bound_already_set;

    union
    {
        struct
        {
            float m_f_val;
            float m_f_max;
            float m_f_min;
            float m_f_step;
            float m_f_opt_backup_value;
        };
        struct
        {
            int m_i_val;
            int m_i_max;
            int m_i_min;
            int m_i_step;
            int m_i_opt_backup_value;
        };
    };
};
