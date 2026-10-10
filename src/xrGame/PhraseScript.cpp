#include "pch_script.h"
#include "PhraseScript.h"
#include "xrScriptEngine/script_engine.hpp"
#include "ai_space.h"
#include "GameObject.h"
#include "script_game_object.h"
#include "InfoPortion.h"
#include "InventoryOwner.h"
#include "ai_debug.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "Actor.h"

//загрузка содержания последовательности тагов в контейнер строк
template <class T>
void LoadSequence(CUIXml* uiXml, const XML_NODE phrase_node, cpcstr tag, T& str_vector)
{
    const size_t tag_num = uiXml->GetNodesNum(phrase_node, tag);
    str_vector.clear();
    str_vector.reserve(tag_num);
    for (size_t i = 0; i < tag_num; ++i)
    {
        cpcstr tag_text = uiXml->Read(phrase_node, tag, i, nullptr);
        str_vector.push_back(tag_text);
    }
}

//загрузка из XML файла
void CDialogScriptHelper::Load(CUIXml* uiXml, const XML_NODE phrase_node)
{
    LoadSequence(uiXml, phrase_node, "precondition", m_Preconditions);
    LoadSequence(uiXml, phrase_node, "action", m_ScriptActions);

    LoadSequence(uiXml, phrase_node, "has_info", m_HasInfo);
    LoadSequence(uiXml, phrase_node, "dont_has_info", m_DontHasInfo);

    LoadSequence(uiXml, phrase_node, "give_info", m_GiveInfo);
    LoadSequence(uiXml, phrase_node, "disable_info", m_DisableInfo);
}

bool CDialogScriptHelper::CheckInfo(const CInventoryOwner* pOwner) const
{
    VERIFY(pOwner);

    for (const auto& info_id : m_HasInfo)
    {
        if (!Actor()->HasInfo(info_id))
        {
#ifdef DEBUG
            if (psAI_Flags.test(aiDialogs))
                Msg("----rejected: [%s] has info %s", pOwner->Name(), info_id.c_str());
#endif
            return false;
        }
    }

    for (const auto& info_id : m_DontHasInfo)
    {
        if (Actor()->HasInfo(info_id))
        {
#ifdef DEBUG
            if (psAI_Flags.test(aiDialogs))
                Msg("----rejected: [%s] dont has info %s", pOwner->Name(), info_id.c_str());
#endif
            return false;
        }
    }
    return true;
}

void CDialogScriptHelper::TransferInfo(const CInventoryOwner* pOwner) const
{
    VERIFY(pOwner);

    for (const auto& info_id : m_GiveInfo)
        Actor()->TransferInfo(info_id, true);

    for (const auto& info_id : m_DisableInfo)
        Actor()->TransferInfo(info_id, false);
}

pcstr CDialogScriptHelper::GetScriptText(cpcstr str_to_translate, const CGameObject* pSpeakerGO1,
    const CGameObject* pSpeakerGO2, cpcstr dialog_id, cpcstr phrase_id) const
{
    if (m_sScriptTextFunc.empty())
        return str_to_translate;

    luabind::functor<pcstr> lua_function;

    const bool functor_exists = GEnv.ScriptEngine->functor(m_sScriptTextFunc.c_str(), lua_function);
    R_ASSERT3_CURE(functor_exists, "Cannot find phrase script text", m_sScriptTextFunc.c_str(),
    {
        return str_to_translate;
    });

    cpcstr res = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
    return res;
}

bool CDialogScriptHelper::Precondition(const CGameObject* pSpeakerGO, LPCSTR dialog_id, LPCSTR phrase_id) const
{
    bool predicate_result = true;

    if (!CheckInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO)))
    {
#ifdef DEBUG
        if (psAI_Flags.test(aiDialogs))
            Msg("dialog [%s] phrase[%s] rejected by CheckInfo", dialog_id, phrase_id);
#endif
        return false;
    }

    for (const auto& precondition : Preconditions())
    {
        if (precondition.empty())
        {
            Msg("! Missing precondition name for phrase[%] dialog[%s]. Speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO->cNameSect().c_str());
            continue;
        }

        luabind::functor<bool> lua_function;
        if (!GEnv.ScriptEngine->functor(precondition.c_str(), lua_function))
        {
            Msg("! Cannot find phrase precondition[%s] for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                precondition.c_str(), phrase_id, dialog_id, pSpeakerGO->cNameSect().c_str());
            continue;
        }

        predicate_result = lua_function(pSpeakerGO->lua_game_object());
        if (!predicate_result)
        {
#ifdef DEBUG
            if (psAI_Flags.test(aiDialogs))
                Msg("dialog [%s] phrase[%s] rejected by script predicate", dialog_id, phrase_id);
#endif
            break;
        }
    }
    return predicate_result;
}

void CDialogScriptHelper::Action(const CGameObject* pSpeakerGO, LPCSTR dialog_id, LPCSTR phrase_id) const
{
    for (const auto& action : Actions())
    {
        if (action.empty())
        {
            Msg("! Missing action name for phrase[%] dialog[%s]. Speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO->cNameSect().c_str());
            continue;
        }

        luabind::functor<void> lua_function;
        if (!GEnv.ScriptEngine->functor(action.c_str(), lua_function))
        {
            Msg("! Cannot find action function[%s] for phrase[%s] dialog[%s]. Speaker[%s].",
                action.c_str(), phrase_id, dialog_id, pSpeakerGO->cNameSect().c_str());
            continue;
        }

        lua_function(pSpeakerGO->lua_game_object(), dialog_id);
    }
    TransferInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO));
}

bool CDialogScriptHelper::Precondition(const CGameObject* pSpeakerGO1, const CGameObject* pSpeakerGO2, LPCSTR dialog_id,
    LPCSTR phrase_id, LPCSTR next_phrase_id) const
{
    bool predicate_result = true;

    if (!CheckInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO1)))
    {
#ifdef DEBUG
        if (psAI_Flags.test(aiDialogs))
            Msg("dialog [%s] phrase[%s] rejected by CheckInfo", dialog_id, phrase_id);
#endif
        return false;
    }
    for (const auto& precondition : Preconditions())
    {
        if (precondition.empty())
        {
            Msg("! Missing precondition name for phrase[%] dialog[%s]. First speaker[%s], second speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        luabind::functor<bool> lua_function;
        if (!GEnv.ScriptEngine->functor(precondition.c_str(), lua_function))
        {
            Msg("! Cannot find phrase precondition[%s] for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                precondition.c_str(), phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(),
            dialog_id, phrase_id, next_phrase_id);
        if (!predicate_result)
        {
#ifdef DEBUG
            if (psAI_Flags.test(aiDialogs))
                Msg("dialog [%s] phrase[%s] rejected by script predicate", dialog_id, phrase_id);
#endif
            break;
        }
    }
    return predicate_result;
}

void CDialogScriptHelper::Action(
    const CGameObject* pSpeakerGO1, const CGameObject* pSpeakerGO2, LPCSTR dialog_id, LPCSTR phrase_id) const
{
    TransferInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO1));

    for (const auto& action : Actions())
    {
        if (action.empty())
        {
            Msg("! Missing action name for phrase[%] dialog[%s]. First speaker[%s], second speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        luabind::functor<void> lua_function;
        if (!GEnv.ScriptEngine->functor(action.c_str(), lua_function))
        {
            Msg("! Cannot find action function[%s] for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                action.c_str(), phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
    }
}
