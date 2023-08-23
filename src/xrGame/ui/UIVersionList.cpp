#include "StdAfx.h"
#include "UIVersionList.h"
#include "VersionSwitcher.h"
#include "xrUICore/ListBox/UIListBoxItem.h"

CUIVersionList::CUIVersionList()
    : CUIWindow("CUIVersionList")
{
    itemsCount = 0;

    versionsList = xr_new<CUIListBox>();
    frame = xr_new<CUIFrameWindow>("Frame");
    header = xr_new<CUIFrameLineWnd>("Header");

    versionsList->SetAutoDelete(true);
    frame->SetAutoDelete(true);
    header->SetAutoDelete(true);

    AttachChild(versionsList);
    AttachChild(frame);
    AttachChild(header);
}

void CUIVersionList::InitFromXml(CUIXml& xml_doc, const char* path)
{
    CUIXmlInit::InitWindow(xml_doc, path, 0, this);
    string256 buf;
    CUIXmlInit::InitListBox(xml_doc, strconcat(sizeof(buf), buf, path, ":list_versions"), 0, versionsList);
    CUIXmlInit::InitFrameLine(xml_doc, strconcat(sizeof(buf), buf, path, ":header"), 0, header);
    CUIXmlInit::InitFrameWindow(xml_doc, strconcat(sizeof(buf), buf, path, ":frame"), 0, frame);

    UpdateVersionList();
}

void CUIVersionList::UpdateVersionList()
{
    versionsList->Clear();

    itemsCount = CVersionSwitcher::GetVerCount();

    for (size_t i = 0; i < itemsCount; ++i)
    {
        const SVersionDescription desc = CVersionSwitcher::GetVerDesc(i);

        CUIListBoxItem* itm = versionsList->AddTextItem(desc.name.c_str());
        itm->SetData(reinterpret_cast<void*>(i));
        itm->Enable(true);
    }
}

const SVersionDescription* CUIVersionList::GetCurrentItem() const
{
    CUIListBoxItem* itm = versionsList->GetSelectedItem();
    if (!itm)
        return nullptr;
    return &CVersionSwitcher::GetVerDesc(reinterpret_cast<size_t>(itm->GetData()));
}

pcstr CUIVersionList::GetCurrentVersionName() const
{
    const SVersionDescription* desc = GetCurrentItem();
    return desc ? desc->name.c_str() : "";
}

pcstr CUIVersionList::GetCurrentVersionDescr() const
{
    const SVersionDescription* desc = GetCurrentItem();
    return desc ? desc->description.c_str() : "";
}

size_t CUIVersionList::GetItemsCount() const { return itemsCount; }

void CUIVersionList::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == versionsList)
    {
        switch (msg)
        {
        case LIST_ITEM_CLICKED:
        {
            GetMessageTarget()->SendMessage(this, LIST_ITEM_CLICKED, pData);
            break;
        }
        case WINDOW_LBUTTON_DB_CLICK:
        {
            GetMessageTarget()->SendMessage(this, WINDOW_LBUTTON_DB_CLICK, pData);
            break;
        }
        }
    }
}

void CUIVersionList::SwitchToSelectedVersion() const
{
    CUIListBoxItem* itm = versionsList->GetSelectedItem();
    if (itm)
    {
        size_t idx = reinterpret_cast<size_t>(itm->GetData());
        CVersionSwitcher::SwitchToGameVer(idx, CVersionSwitcher::SWITCH_TO_MAINMENU);
    }
}
