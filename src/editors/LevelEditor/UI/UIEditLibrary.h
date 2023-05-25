#pragma once

class UIEditLibrary : public XrUI
{
public:
	UIEditLibrary();
	virtual ~UIEditLibrary();

	static void Update();
	static void Show();
	static void Close();
	static void OnRender();

	ImTextureID m_NullTexture;
	ImTextureID m_RealTexture;

	void OnItemFocused(ListItem *item);

private:
	static UIEditLibrary *Form;

	virtual void Draw();
	void DrawObjects();

	void DrawRightBar();
	void DrawObject(CCustomObject *obj, const char *name);
	void InitObjects();
	void OnPropertiesClick();
	void OnMakeThmClick();
	void OnPreviewClick();

	void MakeLOD(bool highQuality);
	void GenerateLOD(RStringVec &props, bool bHighQuality);

	void RefreshSelected();
	void ChangeReference(const RStringVec &items);
	bool SelectionToReference(ListItemsVec *props);

	UIItemListForm* m_ObjectList;
	UIPropertiesForm* m_Props;
	LPCSTR m_Current;
	bool m_Preview;
	ListItem* m_Selected;

	bool m_SelectLods;
	bool m_HighQualityLod;

	xr_vector<CSceneObject *> m_pEditObjects;

	/*

	static IC bool IsOpen() { return Form; }
private:
	static UIObjectList* Form;
private:
	void DrawObjects();
	void DrawObject(CCustomObject* obj, const char* name);
private:
	ObjClassID m_cur_cls;
	enum EMode
	{
		M_All,
		M_Visible,
		M_Inbvisible
	};
	EMode m_Mode;
	CCustomObject* m_SelectedObject;
	string_path m_Filter;*/
};
