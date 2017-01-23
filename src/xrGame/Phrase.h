#pragma once

#include "PhraseScript.h"

class CPhraseDialog;
class CGameObject;

class CPhrase
{
private:
	friend CPhraseDialog;
public:
							CPhrase			();
	virtual					~CPhrase		();

	void					SetText			(LPCSTR text)			{m_text = text;}
	LPCSTR					GetText			()	const;

	LPCSTR					GetScriptText	()	const;

	void					SetID			(const shared_str& id)	{m_ID = id;}
	const shared_str&		GetID			()	const				{return m_ID;}
	bool					IsFinalizer		()	const				{return m_b_finalizer;}
	void					SetFinalizer	(bool b)				{m_b_finalizer=b;}
	int						GoodwillLevel	()	const				{return m_iGoodwillLevel;}

	bool					IsDummy			()	const;
	CDialogScriptHelper*	GetScriptHelper	()						{return &m_ScriptHelper;};

	int						GetGoodwillLevel() const				{return m_iGoodwillLevel;}
	void					SetGoodwillLevel(int v)					{m_iGoodwillLevel = v;}

protected:
	//уникальный индекс в списке фраз диалога
	shared_str		m_ID;
	//текстовое представление фразы
	xr_string		m_text;
	xr_string		m_script_text_id;	
	xr_string		m_script_text_val;
	//минимальный уровень благосклоггости, необходимый для того
	//чтоб фразу можно было сказать
	int				m_iGoodwillLevel;
	bool			m_b_finalizer;
	//для вызова скриптовых функций
	CDialogScriptHelper	m_ScriptHelper;
};