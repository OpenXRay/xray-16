#pragma once

class XREPROPS_API UITreeItem
{
public:
	UITreeItem(shared_str Name);
	virtual ~UITreeItem();

	UITreeItem* AppendItem(const char* Path,char PathChar = '\\');
	UITreeItem* FindItem(const char* Path, char PathChar = '\\');
	xr_vector<UITreeItem*> Items;
	UITreeItem* Owner;
	shared_str Name;
protected:
	virtual UITreeItem* CreateItem(shared_str Name);
};
