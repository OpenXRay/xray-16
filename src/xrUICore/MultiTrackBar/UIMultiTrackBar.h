#pragma once
#include "xrUICore/Options/UIOptionsItem.h"
#include "xrUICore/InteractiveBackground/UI_IB_Static.h"

class CUI3tButton;

class CUITrackBar;

class XRUICORE_API CUIMultiTrackBar final : public CUI_IB_FrameLineWnd, public CUIOptionsItem
{
public:
    enum SaveDataTypes
    {
        SDT_Fvector3,
        SDT_Fvector4,
        SDT_INVALID_DATA_TYPE
    };

    struct ChildTrackBarData {
        int isInverted;
        float min;
        float max;
        float step;
        int displayOrder;
        pcstr displayLabel;
        float displayScale;
        float defaultValue;
    };

    CUIMultiTrackBar();
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
    void InitTrackBars(Fvector2 pos, Fvector2 size, xr_vector<ChildTrackBarData>& trackBarData);
    virtual void Enable(bool status);

    void SetDefaultInvert(bool v) { m_b_invert = v; }
    bool GetDefaultInvert() const { return m_b_invert; };

    void SetDefaultMin(float v) { m_default_min = v; }
    float GetDefaultMin() const { return m_default_min; };
    void SetDefaultMax(float v) { m_default_max = v; }
    float GetDefaultMax() const { return m_default_max; };

    void SetDefaultStep(float step) { m_default_step = step; }
    float GetDefaultStep() { return m_default_step; }

    void SetType(bool b_float) { m_b_is_float = b_float; };
    void SetBoundReady(bool b_ready) { m_b_bound_already_set = b_ready; };

    Fvector4 GetValue() { return m_f_val; }
    void SetChildCount(int count) { childCount = count; }

    SaveDataTypes GetSaveDataType() { return saveDataType; }
    void SetSaveDataType(SaveDataTypes type)
    {
        saveDataType = type;
        Msg("Yohji debug - set save data type %d", saveDataType);
    }

    pcstr GetDebugType() override { return "CUIMultiTrackBar"; }

    CUIStatic* m_static;
    shared_str m_static_format;

    CUITrackBar* GetTrackBarAtIdx(int idx)
    {
        return m_pSliders->at(idx);
    }

    void UpdatePos();

    void SetDisplayModifier(float v) { m_default_display_modifier = v; }
    float GetDisplayModifier() { return m_default_display_modifier; }

    xr_vector<ChildTrackBarData> childTrackBarData;

protected:
    xr_vector<CUITrackBar*>* m_pSliders{};
    bool m_b_invert;
    bool m_b_is_float;
    bool m_b_mouse_capturer;
    bool m_b_bound_already_set;

    int childCount;
    SaveDataTypes saveDataType{ SDT_INVALID_DATA_TYPE };

    float m_default_step;
    float m_default_min;
    float m_default_max;
    float m_default_display_modifier{ 1.f };
    Fvector4 m_f_val;
    Fvector4 m_f_opt_backup_value;
};
