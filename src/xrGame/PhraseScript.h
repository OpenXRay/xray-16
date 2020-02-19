///////////////////////////////////////////////////////////////
// PhraseScript.h
// классы для связи диалогов со скриптами
///////////////////////////////////////////////////////////////

#include "InfoPortionDefs.h"

#pragma once

class CGameObject;
class CInventoryOwner;
class CUIXml;

class CDialogScriptHelper
{
public:
    void Load(CUIXml* ui_xml, XML_NODE phrase_node);

    bool Precondition(const CGameObject* pSpeaker, const char* dialog_id, const char* phrase_id) const;
    void Action(const CGameObject* pSpeaker, const char* dialog_id, const char* phrase_id) const;

    bool Precondition(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2, const char* dialog_id, const char* phrase_id,
        const char* next_phrase_id) const;
    void Action(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2, const char* dialog_id, const char* phrase_id) const;

    using PRECONDITION_VECTOR = xr_vector<shared_str>;
    const PRECONDITION_VECTOR& Preconditions() const { return m_Preconditions; }
    using ACTION_NAME_VECTOR = xr_vector<shared_str>;
    const ACTION_NAME_VECTOR& Actions() const { return m_ScriptActions; }
    void AddPrecondition(const char* str);
    void AddAction(const char* str);
    void AddHasInfo(const char* str);
    void AddDontHasInfo(const char* str);
    void AddGiveInfo(const char* str);
    void AddDisableInfo(const char* str);
    void SetScriptText(const char* str) { m_sScriptTextFunc = str; }
    const char* GetScriptText(const char* str_to_translate, const CGameObject* pSpeakerGO1, const CGameObject* pSpeakerGO2,
        const char* dialog_id, const char* phrase_id);

protected:
    //загрузка содержания последовательности тагов в контейнер строк
    template <class T>
    void LoadSequence(CUIXml* ui_xml, XML_NODE phrase_node, const char* tag, T& str_vector);

    //манипуляции с информацией во время вызовов Precondition и Action
    virtual bool CheckInfo(const CInventoryOwner* pOwner) const;
    virtual void TransferInfo(const CInventoryOwner* pOwner) const;

    //имя скриптовой функции, которая возвращает какой-то текст
    shared_str m_sScriptTextFunc;

    //скриптовые действия, которые активируется после того как
    //говорится фраза
    ACTION_NAME_VECTOR m_ScriptActions;

    using INFO_VECTOR = xr_vector<shared_str>;

    INFO_VECTOR m_GiveInfo;
    INFO_VECTOR m_DisableInfo;

    //список скриптовых предикатов, выполнение, которых необходимо
    //для того чтоб фраза стала доступной

    PRECONDITION_VECTOR m_Preconditions;
    //проверка наличия/отсутствия информации
    INFO_VECTOR m_HasInfo;
    INFO_VECTOR m_DontHasInfo;
};
