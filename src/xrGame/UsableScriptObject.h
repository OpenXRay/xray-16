#pragma once

#include "script_space_forward.h"

class CGameObject;

class CUsableScriptObject
{
public:
							CUsableScriptObject		();
							~CUsableScriptObject	();
		virtual bool		use						(CGameObject* who_use);
	
		//строчка по€вл€юща€с€ при наведении на объект (если NULL, то нет)
		virtual LPCSTR		tip_text				();
		void				set_tip_text			(LPCSTR new_text);
		virtual void		set_tip_text_default	();

		//можно ли использовать объект стандартным (не скриптовым) образом
		bool				nonscript_usable		();
		void				set_nonscript_usable	(bool usable);
private:
		shared_str			m_sTipText;
		bool				m_bNonscriptUsable;
};
