#pragma once

#include "shared_data.h"
#include "Phrase.h"
#include "xrAICore/Navigation/graph_abstract.h"
#include "PhraseDialogDefs.h"
#include "xml_str_id_loader.h"

typedef CGraphAbstract<CPhrase*, float, shared_str> CPhraseGraph;

struct SPhraseDialogData : CSharedResource
{
    SPhraseDialogData();
    virtual ~SPhraseDialogData();

    //заголовок диалога, если NULL, то принимается за стартовую фразу
    shared_str m_sCaption;

    //однонаправленый граф фраз
    //описывает все возможные варианты развития диалога
    CPhraseGraph m_PhraseGraph;

    //список скриптовых предикатов, выполнение, которых необходимо
    //для начала диалога
    CDialogScriptHelper m_ScriptDialogHelper;

    //произвольное число - приоритет диалога (0 по умолчанию), может быть отрицательным
    //в окне выбора у актера диалоги будут сортироваться по этому значению от меньшего (снизу) к большему (сверху)
    int m_iPriority;
};

using PHRASE_VECTOR = xr_vector<CPhrase*>;

class CPhraseDialog;
class CPhraseDialogManager;

class CPhraseDialog : public CSharedClass<SPhraseDialogData, shared_str, false>,
                      public CXML_IdToIndex<CPhraseDialog>,
                      public intrusive_base
{
    typedef CSharedClass<SPhraseDialogData, shared_str, false> inherited_shared;
    typedef CXML_IdToIndex<CPhraseDialog> id_to_index;

    friend id_to_index;

public:
    CPhraseDialog();
    virtual ~CPhraseDialog();

    CPhraseDialog(const CPhraseDialog& pharase_dialog) { *this = pharase_dialog; }
    CPhraseDialog& operator=(const CPhraseDialog& pharase_dialog)
    {
        *this = pharase_dialog;
        return *this;
    }

    virtual void Load(shared_str dialog_id);

    //связь диалога между двумя DialogManager
    virtual void Init(CPhraseDialogManager* speaker_first, CPhraseDialogManager* speaker_second);

    IC bool IsInited() const { return ((FirstSpeaker() != NULL) && (SecondSpeaker() != NULL)); }
    //реинициализация диалога
    virtual void Reset();

    //список предикатов начала диалога
    virtual bool Precondition(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2);

    //список доступных в данный момент фраз
    virtual const PHRASE_VECTOR& PhraseList() const { return m_PhraseVector; }
    bool allIsDummy();
    //сказать фразу и перейти к следующей стадии диалога
    //если вернули false, то считаем, что диалог закончился
    //(сделано статическим, так как мы должны передавать имеенно DIALOG_SHARED_PTR&,
    //а не обычный указатель)
    static bool SayPhrase(DIALOG_SHARED_PTR& phrase_dialog, const shared_str& phrase_id);

    LPCSTR GetPhraseText(const shared_str& phrase_id, bool current_speaking = true);
    LPCSTR GetLastPhraseText() { return GetPhraseText(m_SaidPhraseID, false); }
    const shared_str& GetDialogID() const { return m_DialogId; }
    //заголовок, диалога, если не задан, то 0-я фраза
    const shared_str& GetLastPhraseID() { return m_SaidPhraseID; }
    LPCSTR DialogCaption();
    int Priority();

    bool IsFinished() const { return m_bFinished; }
    IC CPhraseDialogManager* FirstSpeaker() const { return m_pSpeakerFirst; }
    IC CPhraseDialogManager* SecondSpeaker() const { return m_pSpeakerSecond; }
    //кто собирается говорить и кто слушать
    CPhraseDialogManager* CurrentSpeaker() const;
    CPhraseDialogManager* OtherSpeaker() const;
    //кто последний сказал фразу
    CPhraseDialogManager* LastSpeaker() const { return m_bFirstIsSpeaking ? SecondSpeaker() : FirstSpeaker(); }
    IC bool FirstIsSpeaking() const { return m_bFirstIsSpeaking; }
    IC bool SecondIsSpeaking() const { return !m_bFirstIsSpeaking; }
    IC bool IsWeSpeaking(CPhraseDialogManager* dialog_manager) const
    {
        return (FirstSpeaker() == dialog_manager && FirstIsSpeaking()) ||
            (SecondSpeaker() == dialog_manager && SecondIsSpeaking());
    }
    CPhraseDialogManager* OurPartner(CPhraseDialogManager* dialog_manager) const;

protected:
    //идентификатор диалога
    shared_str m_DialogId;

    // ID последней сказанной фразы в диалоге, "" если такой не было
    shared_str m_SaidPhraseID;
    //диалог закончен
    bool m_bFinished;

    //список указателей на фразы доступные в данный момент
    PHRASE_VECTOR m_PhraseVector;

    //указатели на собеседников в диалоге
    CPhraseDialogManager* m_pSpeakerFirst;
    CPhraseDialogManager* m_pSpeakerSecond;
    bool m_bFirstIsSpeaking;

    const SPhraseDialogData* data() const
    {
        VERIFY(inherited_shared::get_sd());
        return inherited_shared::get_sd();
    }
    SPhraseDialogData* data()
    {
        VERIFY(inherited_shared::get_sd());
        return inherited_shared::get_sd();
    }

    //загрузка диалога из XML файла
    virtual void load_shared(LPCSTR);

    //рекурсивное добавление фраз в граф
    void AddPhrase(CUIXml* pXml, XML_NODE phrase_node, const shared_str& phrase_id, const shared_str& prev_phrase_id);

public:
    CPhrase* AddPhrase(LPCSTR text, const shared_str& phrase_id, const shared_str& prev_phrase_id, int goodwil_level);
    CPhrase* AddPhrase_script(LPCSTR text, LPCSTR phrase_id, LPCSTR prev_phrase_id, int goodwil_level)
    {
        return AddPhrase(text, phrase_id, prev_phrase_id, goodwil_level);
    };
    void SetCaption(LPCSTR str);
    void SetPriority(int val);
    CPhrase* GetPhrase(const shared_str& phrase_id);

protected:
    static void InitXmlIdToIndex();
};
