protected:
	DEF_VECTOR(MESSAGEMENUS, cl_MessageMenu);
	MESSAGEMENUS					m_aMessageMenus;

	virtual		void				AddMessageMenu			(LPCSTR	menu_section, LPCSTR snd_path, LPCSTR team_prefix);
	virtual		void				LoadMessagesMenu		(LPCSTR menus_section);
	virtual		void				DestroyMessagesMenus	();
	virtual		void				HideMessageMenus		();

public:
	virtual		void				OnMessageSelected		(CUISpeechMenu* pMenu, u8 PhraseID);
	virtual		void				OnSpeechMessage			(NET_Packet& P);

	