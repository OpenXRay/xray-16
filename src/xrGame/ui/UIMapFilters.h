#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUICheckButton;

class CUIMapFilters final : public CUIWindow
{
    typedef CUIWindow inherited;

public:
    enum eSpotsFilter
    {
        Treasures,
        QuestNpcs,
        SecondaryTasks,
        PrimaryObjects,

        Filter_Count
    };

private:
    std::array<CUICheckButton*, Filter_Count> m_filters{};
    std::array<bool, Filter_Count> m_filters_state{};
    int m_selected_filter{ -1 };

public:
    CUIMapFilters();

    bool Init(CUIXml& xml);

    pcstr GetDebugType() override { return "CUIMapFilters"; }

    void Reset() override;

    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;
    bool OnControllerAction(int axis, float x, float y, EUIMessages controller_action) override;

    void SendMessage(CUIWindow* pWnd, s16 msg, void* pData) override;

    [[nodiscard]]
    bool IsFilterEnabled(eSpotsFilter filter) const
    {
        return m_filters_state[filter];
    }

    void SetFilterEnabled(eSpotsFilter filter, bool enable);

private:
    CUICheckButton* GetSelectedFilter() const;
    void SelectFilter(bool select, bool next = true);
};

