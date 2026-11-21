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
#define XR_DIALOG_VANILLA  0
#define XR_DIALOG_MONOLITH 1 //Сделано чтобы не усложнять #1373, парсер из monolyth, не совместим с ванилой
#define XR_DIALOG_UPDATE   2 //OGSR-like lua runtime, совместим с ванилой

#define XR_DIALOG_MODE XR_DIALOG_UPDATE   

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

    m_sScriptTextFunc = uiXml->Read(phrase_node, "script_text", 0, "");
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

    if (!pSpeakerGO1 || !pSpeakerGO2)
        return str_to_translate; 

#if XR_DIALOG_MODE == XR_DIALOG_MONOLITH
    ::luabind::functor<pcstr> lua_function;
    string256 str = {0};
    xr_sprintf(str, sizeof(str), m_sScriptTextFunc.c_str());
    LPCSTR v1 = strchr(str, '(');
    LPCSTR v2 = strchr(str, ')');
    if (v1 && v2 && (v1 < v2))
    {
        str[v1 - str] = '\0';
        str[v2 - str] = '\0';
        LPCSTR parameters_table_str = v1 + 1;
        int n = _GetItemCount(parameters_table_str, ':');
        for (int k = 0; k < n; k++)
        {
            string64 tmp;
            _GetItem(parameters_table_str, k, tmp, sizeof(tmp), ':');
            parameters_table[k + 1] = tmp;
        }
    }
    bool functor_exists = GEnv.ScriptEngine->functor(str, lua_function);
    THROW3(functor_exists, "Cannot find phrase script text ", m_sScriptTextFunc.c_str());
    return lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, "", parameters_table);

#elif XR_DIALOG_MODE == XR_DIALOG_UPDATE
    xr_string ScriptTextString(m_sScriptTextFunc.c_str()); //-V808
    
    // Переименовали в lua_function_upd, чтобы избежать warning C4458 (shadowing) с outer-объявлением
    ::luabind::functor<const char*> lua_function_upd;
    if (GEnv.ScriptEngine->functor(ScriptTextString.c_str(), lua_function_upd))
    {
        return lua_function_upd(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
    }
    else
    { 
        ::luabind::functor<luabind::object> loadstring_functor;
        GEnv.ScriptEngine->functor("loadstring", loadstring_functor);
        ScriptTextString = "return " + ScriptTextString;
        ::luabind::object ret_obj = loadstring_functor(ScriptTextString.c_str());
        auto ret_func = luabind::object_cast<luabind::functor<const char*>>(ret_obj); 
        
        if (!ret_func) {
            Msg("! Cannot compile script_text to functor: [%s]", m_sScriptTextFunc.c_str());
            return str_to_translate;
        }

        return ret_func(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
    }
#else
    ::luabind::functor<pcstr> lua_function;
    const bool functor_exists = GEnv.ScriptEngine->functor(m_sScriptTextFunc.c_str(), lua_function);
    R_ASSERT3_CURE(functor_exists, "Cannot find phrase script text", m_sScriptTextFunc.c_str(),
    {
        return str_to_translate;
    });

    return lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
#endif
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
            Msg("! Missing precondition name for phrase[%s] dialog[%s]. Speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO->cNameSect().c_str());
            continue;
        }

        ::luabind::functor<bool> lua_function;
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
            Msg("! Missing action name for phrase[%s] dialog[%s]. Speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO->cNameSect().c_str());
            continue;
        }

        ::luabind::functor<void> lua_function;
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

void GetLuaFunctionStringAndHeaderFlag(char *str, char *dst, int dst_size, bool &is_positive)
{
	if (str && (str[0] != 0))
	{
		switch (str[0])
		{
		case '!':
			xr_sprintf(dst, dst_size, str + 1);
			is_positive = false;
			break;
		case '=':
			xr_sprintf(dst, dst_size, str + 1);
			is_positive = true;
			break;
		default:
			is_positive = true;
			xr_sprintf(dst, dst_size, str);
			break;
		}
	}
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
            Msg("! Missing precondition name for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        ::luabind::functor<bool> lua_function;

#if XR_DIALOG_MODE == XR_DIALOG_MONOLITH 
    ::luabind::object parameters_table = ::luabind::newtable(GEnv.ScriptEngine->lua());
		string256 str = {0};
		xr_sprintf(str, sizeof(str), precondition.c_str());
		LPCSTR v1 = strchr(str, '(');
		LPCSTR v2 = strchr(str, ')');
		if (v1 && v2 && (v1 < v2))
		{
			str[v1 - str] = '\0';
			str[v2 - str] = '\0';
			LPCSTR parameters_table_str = v1 + 1;
			int n = _GetItemCount(parameters_table_str, ':');
			for (int k = 0; k < n; k++)
			{
				string64 tmp;
				_GetItem(parameters_table_str, k, tmp, sizeof(tmp), ':');
				parameters_table[k + 1] = tmp;
			}
		}
		bool is_positive = true;
		string256 lua_function_str = {0};
		GetLuaFunctionStringAndHeaderFlag(str, lua_function_str, sizeof(lua_function_str), is_positive);
		bool functor_exists = GEnv.ScriptEngine->functor(lua_function_str, lua_function);

		if(parameters_table.is_valid()){
		    try
		    {
			    predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, next_phrase_id, parameters_table);
		    }
		    catch (...)
		    {
		    }
        }
        else {
            predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
        };
		predicate_result = (is_positive == true) ? predicate_result : !predicate_result;
#elif  XR_DIALOG_MODE == XR_DIALOG_UPDATE
        xr_string ActionString(precondition.c_str()); 
        if (::luabind::functor<bool> lua_function; GEnv.ScriptEngine->functor(ActionString.c_str(), lua_function)) // Обычный функтор
            predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, next_phrase_id);
        else{
            ::luabind::functor<luabind::object> loadstring_functor;
            GEnv.ScriptEngine->functor("loadstring", loadstring_functor);
            ActionString = "return " + ActionString;
            ::luabind::object ret_obj = loadstring_functor(ActionString.c_str()); 
            auto ret_func = ::luabind::object_cast<::luabind::functor<bool>>(ret_obj); 
            if (!ret_func) {
                Msg("Loadstring returns nil for code: [%s]",ActionString.c_str());
                continue;
            }
            predicate_result = ret_func(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, next_phrase_id);
        }
#else
        if (!GEnv.ScriptEngine->functor(precondition.c_str(), lua_function))
        {
            Msg("! Cannot find phrase precondition[%s] for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                precondition.c_str(), phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(),
            dialog_id, phrase_id, next_phrase_id);
#endif
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
            Msg("! Missing action name for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }
        ::luabind::functor<void> lua_function;

#if XR_DIALOG_MODE == XR_DIALOG_MONOLITH 
        ::luabind::object parameters_table = ::luabind::newtable(GEnv.ScriptEngine->lua());
		string256 str = {0};
		xr_sprintf(str, sizeof(str), action.c_str());
		LPCSTR v1 = strchr(str, '(');
		LPCSTR v2 = strchr(str, ')');
		if (v1 && v2 && (v1 < v2))
		{
			str[v1 - str] = '\0';
			str[v2 - str] = '\0';
			LPCSTR parameters_table_str = v1 + 1;
			int n = _GetItemCount(parameters_table_str, ':');
			for (int k = 0; k < n; k++)
			{
				string64 tmp;
				_GetItem(parameters_table_str, k, tmp, sizeof(tmp), ':');
				parameters_table[k + 1] = tmp;
			}
		}
		bool is_positive = true;
		string256 lua_function_str = {0};
		GetLuaFunctionStringAndHeaderFlag(str, lua_function_str, sizeof(lua_function_str), is_positive);
		bool functor_exists = GEnv.ScriptEngine->functor(lua_function_str, lua_function);
        if(parameters_table.is_valid()){
		    try
		    {
			    lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, "", parameters_table);
		    }
		    catch (...)
		    {
		    }
        }
        else {
            lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
        };

#elif XR_DIALOG_MODE == XR_DIALOG_UPDATE
        xr_string ActionString(action.c_str()); 
        if (::luabind::functor<void> lua_function; GEnv.ScriptEngine->functor(ActionString.c_str(), lua_function)) // Обычный функтор
            lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
        else{
            ::luabind::functor<luabind::object> loadstring_functor;
            GEnv.ScriptEngine->functor("loadstring", loadstring_functor);
            ActionString = "return " + ActionString;
            ::luabind::object ret_obj = loadstring_functor(ActionString.c_str()); 
            auto ret_func = ::luabind::object_cast<::luabind::functor<void>>(ret_obj); 
            if (!ret_func) {
                Msg("Loadstring returns nil for code: [%s]",ActionString.c_str());
                continue;
            }
            ret_func(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
        }
#else
        if (!GEnv.ScriptEngine->functor(action.c_str(), lua_function))
        {
            Msg("! Cannot find action function[%s] for phrase[%s] dialog[%s]. First speaker[%s], second speaker[%s].",
                action.c_str(), phrase_id, dialog_id, pSpeakerGO1->cNameSect().c_str(), pSpeakerGO2->cNameSect().c_str());
            continue;
        }

        lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
#endif
    }
}
