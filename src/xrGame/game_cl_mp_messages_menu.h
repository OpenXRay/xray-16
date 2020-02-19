protected:
using MESSAGEMENUS = xr_vector<cl_MessageMenu>;
MESSAGEMENUS m_aMessageMenus;

virtual void AddMessageMenu(const char* menu_section, const char* snd_path, const char* team_prefix);
virtual void LoadMessagesMenu(const char* menus_section);
virtual void DestroyMessagesMenus();
virtual void HideMessageMenus();

public:
virtual void OnMessageSelected(CUISpeechMenu* pMenu, u8 PhraseID);
virtual void OnSpeechMessage(NET_Packet& P);
