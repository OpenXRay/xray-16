#pragma once
#include "UIDialogWnd.h"
#include "xrUICore/Static/UIStatic.h"
#include "xrUICore/Buttons/UIButton.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "PhraseDialogDefs.h"

class CActor;
class CInventoryOwner;
class CPhraseDialogManager;
class CUITalkDialogWnd;
// class CUITradeWnd;
///////////////////////////////////////
//
///////////////////////////////////////

class CUITalkWnd : public CUIDialogWnd
{
private:
    typedef CUIDialogWnd inherited;
    ref_sound m_sound;
    void PlaySnd(LPCSTR text);
    void StopSnd();

public:
    CUITalkWnd();
    virtual ~CUITalkWnd();

    IC bool playing_sound() { return !!m_sound._feedback(); }
    IC CInventoryOwner* OthersInvOwner() const { return m_pOthersInvOwner; };
    void InitTalkWnd();

    virtual bool StopAnyMove() { return true; }
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    virtual void Draw();
    virtual void Update();

    virtual void Show(bool status);

    void Stop(); // deffered
    void StopTalk();

    void UpdateQuestions();
    void NeedUpdateQuestions();
    //инициализации начального диалога собеседника
    void InitOthersStartDialog();
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    void SwitchToTrade();
    void SwitchToUpgrade();
    void AddIconedMessage(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name);

protected:
    //диалог
    void InitTalkDialog();
    void AskQuestion();

    void SayPhrase(const shared_str& phrase_id);

    // Функции добавления строк в листы вопросов и ответов
public:
    void AddQuestion(const shared_str& text, const shared_str& id, int number, bool b_finalizer);
    void AddAnswer(const shared_str& text, LPCSTR SpeakerName);
    bool b_disable_break;

protected:
    CUITalkDialogWnd* UITalkDialogWnd;

    CActor* m_pActor;
    CInventoryOwner* m_pOurInvOwner;
    CInventoryOwner* m_pOthersInvOwner;

    CPhraseDialogManager* m_pOurDialogManager;
    CPhraseDialogManager* m_pOthersDialogManager;

    bool m_bNeedToUpdateQuestions;

    //текущий диалог, если NULL, то переходим в режим выбора темы
    DIALOG_SHARED_PTR m_pCurrentDialog;
    bool TopicMode();
    void ToTopicMode();
};
