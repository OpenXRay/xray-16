#pragma once
class UIBoneForm : public XrUI
{
public:
	UIBoneForm();
	virtual ~UIBoneForm();
	virtual void Draw();

public:
	static void Update();
	static void Show();

private:
	static UIBoneForm *Form;

private:
	BPVec *m_BoneParts;

private:
	struct ItemList
	{
		ItemList(shared_str Name) : name(Name), select(false) {}
		ItemList() : select(false) {}
		shared_str name;
		bool select;
	};
	xr_vector<ItemList> m_List[4];
	string_path m_Name[4];
	CEditableObject *m_EditObject;

private:
	void Move(int to);
	void FillBoneParts();
	void Save();
	void SaveTo();
	void LoadFrom();
	void ToDefault();
};
