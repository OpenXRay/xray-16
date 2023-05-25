
class UIMinimapEditorForm : public XrUI
{
public:
	UIMinimapEditorForm();
	virtual ~UIMinimapEditorForm();
	virtual void Draw();

	static void Update();
	static void Show();

private:
	static UIMinimapEditorForm* Form;

	ImTextureID m_Texture;
	ImTextureID m_TextureRemove;

	void LoadClick();
	U32Vec m_ImageData;
	u32 m_ImageW;
	u32 m_ImageH;
	u32 m_ImageA;
	Fbox2 map_bb;
	Fbox2 map_bb_loaded;
	Ivector2 image_draw_size;
};
