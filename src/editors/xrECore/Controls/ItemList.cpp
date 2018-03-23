#include "pch.hpp"
#include "ItemList.h"
#include "xrServerEntities/ItemListTypes.h"
#include "FolderLib.h"

#include <sstream>

namespace XRay::Editor::Controls
{
void ItemList::AssignItems(ListItemsVec& newItems, bool fullExpand, bool fullSort /*= false*/)
{
    viewItems->BeginUpdate();

    viewItems->Nodes->Clear();

    for (const auto& item : newItems)
    {
        viewItems->AddItem(BackSlashToSlash(item->Key()));
    }
    /*// begin fill mode
    
    // clear values
    //    if (tvItems->Selected) FHelper.MakeFullName(tvItems->Selected,0,last_selected_item);
    //if (!items->empty())
    //    ClearParams();
    // fill values
    items = &newItems;
    for (auto& prop : *items)
    {
        if (prop->Key.size() && (prop->Key[prop->Key.size() - 1] == '\\'))
        {
            prop->Item = FHelper.AppendFolder(viewItems, prop->Key.c_str(), !flags->is(ilSuppressIcon));
            auto prop_item = prop->Item;
            prop_item->CheckBoxEnabled = false;
            prop_item->UseStyles = true;
            prop_item->MainStyle->TextColor = prop->prop_color;
            prop_item->MainStyle->OwnerProps = true;
            prop_item->MainStyle->Style = ElhsOwnerDraw;
        }
        else
        {
            prop->Item = FHelper.AppendObject(viewItems, prop->Key.c_str(), false, !flags->is(ilSuppressIcon));
            if (!prop->Item)
            {
                Msg("#!Duplicate item name found: '%s'", prop->Key.c_str());
                break;
            }
            TElTreeItem* prop_item = (TElTreeItem*)prop->Item;
            prop_item->ImageIndex = prop->icon_index;
            prop_item->Tag = (int)prop;
            prop_item->UseStyles = true;
            prop_item->CheckBoxEnabled = prop->m_Flags.is(ListItem::flShowCB);
            prop_item->ShowCheckBox = prop->m_Flags.is(ListItem::flShowCB);
            prop_item->CheckBoxState = (TCheckBoxState)prop->m_Flags.is(ListItem::flCBChecked);

            // set flags
            if (prop->m_Flags.is(ListItem::flDrawThumbnail))
            {
                prop_item->Height = 64;
                prop_item->OwnerHeight = !miDrawThumbnails->Checked;
            }
            // set style
            prop_item->MainStyle->OwnerProps = true;
            prop_item->MainStyle->Style = ElhsOwnerDraw;
        }
    }

    // end fill mode
    if (fullExpand)
        viewItems->ExpandAll();

    // folder restore
    if (flags->is(ilFolderStore) && !FolderStore.empty())
    {
        for (TElTreeItem* item = tvItems->Items->GetFirstNode(); item; item = item->GetNext())
        {
            if (item->ChildrenCount)
            {
                xr_string nm;
                FHelper.MakeFullName(item, nullptr, nm);
                FolderStorePairIt it = FolderStore.find(nm);
                if (it != FolderStore.end())
                {
                    SFolderStore& st_item = it->second;
                    if (st_item.expand)
                        item->Expand(false);
                    else
                        item->Collapse(false);
                }
            }
        }
    }

    // sorting
    if (fullSort)
    {
        viewItems->Sort();
    }
    else
    {
        for (auto& prop : *items)
        {
            if (prop->m_Flags.is(ListItem::flSorted))
                ((TElTreeItem*)prop->Item)->Sort(true);
        }
    }

    // expand sel items
    for (RStringVecIt s_it = last_selected_items.begin(); s_it != last_selected_items.end(); s_it++)
        FHelper.ExpandItem(tvItems, **s_it);

    

    for (RStringVecIt s_it = last_selected_items.begin(); s_it != last_selected_items.end(); s_it++)
        FHelper.RestoreSelection(tvItems, **s_it, true);
    */
    
    // restore selection
    viewItems->SelectedNode = nullptr;

    toolStripStatusLabel2->Text = newItems.size().ToString();

    viewItems->EndUpdate();
}
} // namespace XRay::Editor::Controls
