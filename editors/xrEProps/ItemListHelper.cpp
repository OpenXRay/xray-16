#include "stdafx.h"
#pragma hdrstop

#include "ItemListHelper.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

//---------------------------------------------------------------------------
CListHelper 	LHelper_impl;
IListHelper& 	LHelper		(){return LHelper_impl;}
//---------------------------------------------------------------------------

ListItem* CListHelper::FindItem		(ListItemsVec& items,	LPCSTR key)
{
    for (ListItemsIt it=items.begin(); it!=items.end(); it++)
        if ((*it)->key==key) return *it;
    return 0;
}
    
ListItem* CListHelper::CreateItem	(ListItemsVec& items, LPCSTR key, int type, u32 item_flags, void* object)
{
    ListItem* item	= xr_new<ListItem>	(type);
    item->SetName	(key);
    item->m_Object	= object;
    item->m_Flags.set(item_flags,TRUE);
    items.push_back	(item);
    return			item;
}

bool CListHelper::NameAfterEdit(ListItem* sender, LPCSTR value, shared_str& N)
{
    if (0!=AnsiString(N.c_str()).Pos("\\"))	{ N=value; return false; }
	N	= AnsiString(N.c_str()).LowerCase().c_str();
    if (!N.size())	{ N=value; return false; }
	int cnt=_GetItemCount(N.c_str(),'\\');
    if (cnt>1)		{ N=value; return false; }

    VERIFY				(sender);
	TElTreeItem* node  	= (TElTreeItem*)sender->Item(); VERIFY(node);

    for (TElTreeItem* itm=node->GetFirstSibling(); itm; itm=itm->GetNextSibling()){
        if ((itm!=node)&&(itm->Text==AnsiString(N.c_str()))){
        	// елемент с таким именем уже существует
	        N=value;
            return false;
        }
    }
    // all right
    node->Text = N.c_str();
    AnsiString tmp;
	_ReplaceItem(*sender->key,	_GetItemCount(*sender->key,'\\')-1,	N.c_str(),tmp,'\\'); sender->key=tmp.c_str();
    // Имя объекта может быть составным из a\\b\\name
	_ReplaceItem(value,			_GetItemCount(value,'\\')-1,		N.c_str(),tmp,'\\');	N=tmp.c_str();
    
    return true;
}
//---------------------------------------------------------------------------


