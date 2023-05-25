#pragma once

class UIImageEditorForm : public XrUI
{
public:
	UIImageEditorForm();
	virtual ~UIImageEditorForm();
	virtual void Draw();

public:
	static void Update();
	static void Show(bool bImport);
	static void ImportTextures();

private:
	DEFINE_VECTOR(ETextureThumbnail *, THMVec, THMIt);
	DEFINE_MAP(shared_str, ETextureThumbnail *, THMMap, THMMapIt);
	THMMap m_THM_Used;
	THMVec m_THM_Current;
	UIItemListForm *m_ItemList;
	UIPropertiesForm *m_ItemProps;
	FS_FileSet texture_map;
	FS_FileSet modif_map;
	bool bImportMode;
	bool bReadonlyMode;
	static UIImageEditorForm *Form;
	ImTextureID m_Texture;
	ImTextureID m_TextureRemove;

private:
	ETextureThumbnail *FindUsedTHM(const shared_str &name);
	void RegisterModifiedTHM();
	void OnCubeMapBtnClick(ButtonValue *value, bool &bModif, bool &bSafe);
	void OnTypeChange(PropValue *prop);
	void InitItemList();
	void HideLib();
	void UpdateLib();
	void OnItemsFocused(ListItem *item);
	void SaveUsedTHM();
	void UpdateProperties();

private:
	bool m_bFilterImage;
	bool m_bFilterTerrain;
	bool m_bFilterBump;
	bool m_bFilterNormal;
	bool m_bFilterCube;
	bool m_bUpdateProperties;
	void FilterUpdate();
};
