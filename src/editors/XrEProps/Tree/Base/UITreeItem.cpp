#include "stdafx.h"

UITreeItem::UITreeItem(shared_str NewName):Name(NewName)
{
	Owner = nullptr;
}

UITreeItem::~UITreeItem()
{
	for (UITreeItem* Item : Items)	
		xr_delete(Item);	
}

UITreeItem* UITreeItem::AppendItem(const char* Path, char PathChar )
{
	VERIFY(Path && *Path);

	if (PathChar&&strchr(Path, PathChar))
	{
		string_path Name;
		xr_strcpy(Name, Path);
		strchr(Name, PathChar)[0] = 0;
		UITreeItem* Item = FindItem(Name);

		if (!Item)
		{
			Items.push_back(CreateItem(Name));
			Item = Items.back();
			Item->Owner = this;
		}

		return Item->AppendItem(strchr(Path, PathChar) + 1);
	}
	else
	{
		UITreeItem* Item = FindItem(Path);

		if (!Item)
		{
			Items.push_back(CreateItem(Path));
			Item = Items.back();
			Item->Owner = this;
		}

		return Item;
	}
}

UITreeItem* UITreeItem::FindItem(const char* Path, char PathChar )
{
	if (PathChar&&strchr(Path, PathChar))
	{
		string_path Name;
		xr_strcpy(Name, Path);
		strchr(Name, PathChar)[0] = 0;
		
		if (UITreeItem* Item = FindItem(Name))		
			return Item->FindItem(strchr(Path, PathChar) + 1);		
	}
	else
	{
		shared_str FName = Path;

		for (UITreeItem* Item : Items)
		{
			if (Item->Name == FName)			
				return Item;			
		}
	}

	return nullptr;
}

UITreeItem* UITreeItem::CreateItem(shared_str Name)
{
	return xr_new<UITreeItem>(Name);
}
