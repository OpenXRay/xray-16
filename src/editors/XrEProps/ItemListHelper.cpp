#include "stdafx.h"
#pragma hdrstop

#include "ItemListHelper.h"
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CListHelper LHelper_impl;
IListHelper &LHelper() { return LHelper_impl; }
//---------------------------------------------------------------------------

ListItem *CListHelper::FindItem(ListItemsVec &items, LPCSTR key)
{
    for (ListItemsIt it = items.begin(); it != items.end(); it++)
        if ((*it)->key == key)
            return *it;
    return 0;
}

ListItem *CListHelper::CreateItem(ListItemsVec &items, LPCSTR key, int type, u32 item_flags, void *object)
{
    ListItem *item = xr_new<ListItem>(type);
    item->SetName(key);
    item->m_Object = object;
    item->m_Flags.set(item_flags, TRUE);
    items.push_back(item);
    return item;
}

bool CListHelper::NameAfterEdit(ListItem *sender, LPCSTR value, shared_str &N)
{
    if (strstr(N.c_str(), "\\"))
    {
        N = value;
        return false;
    }
    xr_strlwr(N);
    if (!N.size())
    {
        N = value;
        return false;
    }
    int cnt = _GetItemCount(N.c_str(), '\\');
    if (cnt > 1)
    {
        N = value;
        return false;
    }

    for (ListItem *item : sender->Parent->GetItems())
    {
        const char *name = strrchr(item->Key(), '\\');
        if (!name)
            name = item->Key();
        else
            name++;
        if ((item != sender) && (N == name))
        {
            // елемент с таким именем уже существует
            N = value;
            return false;
        }
    }
    string2048 tmp;
    _ReplaceItem(value, _GetItemCount(value, '\\') - 1, N.c_str(), tmp, '\\');
    N = tmp;
    return true;
}
//---------------------------------------------------------------------------
