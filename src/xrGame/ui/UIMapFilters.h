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
    bool m_activated{};

public:
    CUIMapFilters();

    bool Init(CUIXml& xml);

    void Reset() override;

    bool Activate(bool activate);

    bool OnKeyboardAction(int dik, EUIMessages keyboard_action) override;

    void SendMessage(CUIWindow* pWnd, s16 msg, void* pData) override;

    [[nodiscard]]
    bool IsFilterEnabled(eSpotsFilter filter) const;

    void SetFilterEnabled(eSpotsFilter filter, bool enable) const;

    pcstr GetDebugType() override { return "CUIMapFilters"; }

private:
    CUICheckButton* GetSelectedFilter() const;
};

