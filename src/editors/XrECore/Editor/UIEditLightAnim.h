#pragma once
class CLAItem;
class UIEditLightAnim : public XrUI
{
public:
	UIEditLightAnim();
	virtual ~UIEditLightAnim();
	virtual void Draw();

public:
	static void Update();
	static void Show();

private:
	static UIEditLightAnim *Form;

private:
	void UpdateProperties();
	UIItemListForm *m_Items;
	UIPropertiesForm *m_Props;
	ref_texture m_TextureNull;
	ImTextureID m_Texture;
	void InitializeItems();

private:
	void RenderItem();
	ID3DTexture2D *m_ItemTexture;
	CLAItem *m_CurrentItem;
	bool m_Modife;

private:
	void OnCreateKeyClick();
	void OnModified();
	void OnItemFocused(ListItem *);
	bool OnFrameCountAfterEdit(PropValue *v, s32 &val);
	void OnCloneItem(LPCSTR parent_path, LPCSTR new_full_name);
	void OnCreateItem(LPCSTR path);
	void OnRemoveItem(LPCSTR name, EItemType type);
	void OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type);

private:
	void RenderPointer();
	void FillRectPointer(const ImVec4 &rect, u32 color, bool plus_one = false);
	void FrameRectPointer(const ImVec4 &rect, u32 color);
	float m_PointerWeight;
	bool m_PointerResize;
	ID3DTexture2D *m_PointerTexture;
	u32 *m_PointerRawImage;
	int m_PointerValue;
	bool m_RenderAlpha;
};