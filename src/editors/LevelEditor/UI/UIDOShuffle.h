#pragma once
class EDetailManager;
class EImageThumbnail;
class UIDOOneColor;
class UIDOShuffle : public XrUI
{
	friend UIDOOneColor;

public:
	UIDOShuffle();
	virtual ~UIDOShuffle();
	virtual void Draw();
	static void Update();
	static bool GetResult();
	static void Show(EDetailManager *DM);

private:
	static UIDOShuffle *Form;

private:
	EDetailManager *DM;
	void FillData();

private:
	UIPropertiesForm *m_Props;

private:
	EImageThumbnail *m_Thm;
	ref_texture m_TextureNull;
	ImTextureID m_Texture;
	ImTextureID m_RealTexture;

private:
	void OnItemFocused(const char *name);
	xr_vector<xr_string> m_list;
	int m_list_selected;
	bool FindItem(const char *name);

private:
	bool bModif;
	bool m_ChooseObject;

private:
	void ClearIndexForms();
	xr_vector<UIDOOneColor *> m_color_indices;
	bool ApplyChanges(bool msg = true);
};