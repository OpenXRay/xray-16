#include "pch.hpp"
#include "UIOptionsItem.h"
#include "UIOptionsManagerScript.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

void CUIOptionsManagerScript::SetCurrentValues(pcstr group)
{
    CUIOptionsItem::GetOptionsManager()->SetCurrentValues(group);
}

void CUIOptionsManagerScript::SaveBackupValues(pcstr group)
{
    CUIOptionsItem::GetOptionsManager()->SaveBackupValues(group);
}

void CUIOptionsManagerScript::SaveValues(pcstr group) { CUIOptionsItem::GetOptionsManager()->SaveValues(group); }
void CUIOptionsManagerScript::UndoGroup(pcstr group) { CUIOptionsItem::GetOptionsManager()->UndoGroup(group); }
void CUIOptionsManagerScript::OptionsPostAccept() { CUIOptionsItem::GetOptionsManager()->OptionsPostAccept(); }
void CUIOptionsManagerScript::SendMessage2Group(pcstr group, pcstr message)
{
    CUIOptionsItem::GetOptionsManager()->SendMessage2Group(group, message);
}

bool CUIOptionsManagerScript::NeedSystemRestart() { return CUIOptionsItem::GetOptionsManager()->NeedSystemRestart(); }
bool CUIOptionsManagerScript::NeedVidRestart() { return CUIOptionsItem::GetOptionsManager()->NeedVidRestart(); }

SCRIPT_EXPORT(CUIOptionsManagerScript, (), {
    module(luaState)[class_<CUIOptionsManagerScript>("COptionsManager")
                         .def(constructor<>())
                         .def("SaveBackupValues", &CUIOptionsManagerScript::SaveBackupValues)
                         .def("SetCurrentValues", &CUIOptionsManagerScript::SetCurrentValues)
                         .def("SaveValues", &CUIOptionsManagerScript::SaveValues)
                         .def("UndoGroup", &CUIOptionsManagerScript::UndoGroup)
                         .def("OptionsPostAccept", &CUIOptionsManagerScript::OptionsPostAccept)
                         .def("SendMessage2Group", &CUIOptionsManagerScript::SendMessage2Group)
                         .def("NeedSystemRestart", &CUIOptionsManagerScript::NeedSystemRestart)
                         .def("NeedVidRestart", &CUIOptionsManagerScript::NeedVidRestart)];
});
