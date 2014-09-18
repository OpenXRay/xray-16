#include "pch_script.h"
#include "PhraseDialog_script.h"
#include "PhraseDialog.h"

using namespace luabind;

void CDialogScriptHelper::AddPrecondition	(LPCSTR str)
{
	m_Preconditions.push_back	(str);
}
void CDialogScriptHelper::AddAction		(LPCSTR str)
{
	m_ScriptActions.push_back	(str);
}
void CDialogScriptHelper::AddHasInfo		(LPCSTR str)
{
	m_HasInfo.push_back	(str);
}
void CDialogScriptHelper::AddDontHasInfo	(LPCSTR str)
{
	m_DontHasInfo.push_back	(str);
}
void CDialogScriptHelper::AddGiveInfo		(LPCSTR str)
{
	m_GiveInfo.push_back	(str);
}
void CDialogScriptHelper::AddDisableInfo	(LPCSTR str)
{
	m_DisableInfo.push_back	(str);
}


#pragma optimize("s",on)
void CPhraseDialogExporter::script_register(lua_State *L)
{
	module(L)
	[
		class_<CPhrase>("CPhrase")
		.def("GetPhraseScript",		&CPhrase::GetScriptHelper),

		class_<CPhraseDialog>("CPhraseDialog")
		.def("AddPhrase",			&CPhraseDialog::AddPhrase_script ),

		class_<CDialogScriptHelper>("CPhraseScript")
		.def("AddPrecondition",		&CDialogScriptHelper::AddPrecondition)
		.def("AddAction",			&CDialogScriptHelper::AddAction)
		.def("AddHasInfo",			&CDialogScriptHelper::AddHasInfo)
		.def("AddDontHasInfo",		&CDialogScriptHelper::AddDontHasInfo)
		.def("AddGiveInfo",			&CDialogScriptHelper::AddGiveInfo)
		.def("AddDisableInfo",		&CDialogScriptHelper::AddDisableInfo)
		.def("SetScriptText",		&CDialogScriptHelper::SetScriptText)
	];
}
