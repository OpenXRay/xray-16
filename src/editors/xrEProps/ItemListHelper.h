//---------------------------------------------------------------------------
#ifndef ItemListHelperH
#define ItemListHelperH

//---------------------------------------------------------------------------
class CListHelper : public IListHelper
{
	//------------------------------------------------------------------------------
public:
	virtual ListItem *FindItem(ListItemsVec &items, LPCSTR key);
	virtual bool NameAfterEdit(ListItem *sender, LPCSTR value, shared_str &edit_val);

public:
	virtual ListItem *CreateItem(ListItemsVec &items, LPCSTR key, int type, u32 item_flags = 0, void *object = 0);
};
//---------------------------------------------------------------------------
#endif
