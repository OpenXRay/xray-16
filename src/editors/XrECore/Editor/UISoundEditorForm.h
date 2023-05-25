//---------------------------------------------------------------------------

#ifndef SoundLibH
#define SoundLibH
#endif
class UISoundEditorForm : public XrUI
{
public:
	UISoundEditorForm();
	virtual ~UISoundEditorForm();
	virtual void Draw();

public:
	static void Update();
	static void Show();

private:
	void HideLib();
	static UISoundEditorForm *Form;

	void InitItemList();
	void OnItemsFocused(ListItem *item);
	UIPropertiesForm *m_ItemProps;
	UIItemListForm *m_ItemList;

private:
	enum
	{
		flUpdateProperties = (1 << 0),
		flReadOnly = (1 << 1),
	};
	Flags32 m_Flags;
	DEFINE_VECTOR(ESoundThumbnail *, THMVec, THMIt);
	THMVec m_THM_Used;
	THMVec m_THM_Current;
	ESoundThumbnail *FindUsedTHM(LPCSTR name);
	void SaveUsedTHM();
	void DestroyUsedTHM();

	void RegisterModifiedTHM();

	void OnModified();
	void UpdateLib();

	bool bFormLocked;
	BOOL bAutoPlay;

	FS_FileSet modif_map;
	ref_sound m_Snd;

	void AppendModif(LPCSTR nm);

private:
	void OnControlClick(ButtonValue *sender, bool &bModif, bool &bSafe);
	void OnControl2Click(ButtonValue *sender, bool &bModif, bool &bSafe);
	void OnSyncCurrentClick(ButtonValue *sender, bool &bModif, bool &bSafe);
	void OnAttClick(ButtonValue *sender, bool &bModif, bool &bSafe);
	void PlaySound(LPCSTR name, u32 &size, u32 &time);
	void OnAttenuationDraw(CanvasValue *sender);
};
