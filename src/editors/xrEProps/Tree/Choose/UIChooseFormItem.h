#pragma once

class UIChooseFormItem :public UITreeItem
{
public:
	UIChooseFormItem(shared_str Name);
	virtual ~UIChooseFormItem() = default;

	bool bIsFavorite;

	class UIChooseForm* Form;
	int Index;
	SChooseItem* Object;
	shared_str Text;
	bool bSelected;
	bool m_bOpenByDefault;
	void Draw();
	void DrawRoot();
	void Sort();
	void CheckFavorited();
	void SetFavorite(bool bFavorite);
	void FillFavorited(xr_vector< SChooseItem*>& selected);
	void CheckFavorited(xr_vector< SChooseItem*>& selected);
	void ClearSelection();
	void Selected(int Start, int End);
	void SelectedToFavorite(bool Favorite);
	void OpenParentItems(const char* path, char delimiter = '\\');

protected:
	bool CheckFilter();
	virtual UITreeItem* CreateItem(shared_str Name);
	bool m_bIsMixed;
};
