
#pragma once
#include "../../xrServerEntities/script_export_space.h"

class CUIOptionsManagerScript 
{
public:
	void 		SaveBackupValues		(LPCSTR group);
	void 		SetCurrentValues		(LPCSTR group);
	void 		SaveValues				(LPCSTR group);
	void 		UndoGroup				(LPCSTR group);
	void 		OptionsPostAccept		();
	void 		SendMessage2Group		(LPCSTR group, LPCSTR message);
	bool 		NeedSystemRestart		();
	bool 		NeedVidRestart			();
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
