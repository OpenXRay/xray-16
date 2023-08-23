#pragma once
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/ListBox/UIListBox.h"

struct SVersionDescription;

class CUIVersionList final : public CUIWindow
{
    CUIListBox* versionsList;
    CUIFrameLineWnd* header;
    CUIFrameWindow* frame;

    size_t itemsCount;

public:
    CUIVersionList();
    void InitFromXml(CUIXml& xml_doc, const char* path);
    pcstr GetCurrentVersionName() const;
    pcstr GetCurrentVersionDescr() const;
    void SwitchToSelectedVersion() const;
    const SVersionDescription* GetCurrentItem() const;
    size_t GetItemsCount() const;

    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData);

    pcstr GetDebugType() override { return "CUIVersionList"; }

private:
    void UpdateVersionList();
};
