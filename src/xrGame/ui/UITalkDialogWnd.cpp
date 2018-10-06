#include "StdAfx.h"
#include "UITalkDialogWnd.h"

#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "xrUICore/ScrollView/UIScrollView.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UITalkWnd.h"
#include "UIInventoryUtilities.h"
#include "xrUICore/Buttons/UIBtnHint.h"

#include "game_news.h"
#include "Level.h"
#include "Actor.h"
#include "alife_registry_wrappers.h"

#define TALK_XML "talk.xml"

CUITalkDialogWnd::CUITalkDialogWnd() : m_pNameTextFont(NULL)
{
    m_ClickedQuestionID = "";
    mechanic_mode = false;
}
CUITalkDialogWnd::~CUITalkDialogWnd() { xr_delete(m_uiXml); }
void CUITalkDialogWnd::InitTalkDialogWnd()
{
    m_uiXml = new CUIXml();
    m_uiXml->Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, TALK_XML);
    CUIXmlInit ml_init;

    CUIXmlInit::InitWindow(*m_uiXml, "main", 0, this);

    //	CUIXmlInit::InitStatic		(*m_uiXml, "right_character_icon", 0, &UIOurIcon);

    //	CUIXmlInit::InitStatic		(*m_uiXml, "left_character_icon", 0, &UIOthersIcon);

    //	UIOurIcon.AttachChild		(&UICharacterInfoLeft);
    //	UICharacterInfoLeft.InitCharacterInfo(Fvector2().set(0,0), UIOurIcon.GetWndSize(), "talk_character.xml");

    //	UIOthersIcon.AttachChild	(&UICharacterInfoRight);
    //	UICharacterInfoRight.InitCharacterInfo(Fvector2().set(0,0), UIOthersIcon.GetWndSize(), "talk_character.xml");

    //	AttachChild					(&UIOurIcon);
    //	AttachChild					(&UIOthersIcon);

    // Фрейм с нащими фразами
    //	AttachChild					(&UIDialogFrameBottom);
    //	CUIXmlInit::InitStatic		(*m_uiXml, "frame_bottom", 0, &UIDialogFrameBottom);

    //основной фрейм диалога
    //	AttachChild					(&UIDialogFrameTop);
    //	CUIXmlInit::InitStatic		(*m_uiXml, "frame_top", 0, &UIDialogFrameTop);

    //Ответы
    UIAnswersList = new CUIScrollView();
    UIAnswersList->SetAutoDelete(true);
    //	UIDialogFrameTop.AttachChild(UIAnswersList);
    AttachChild(UIAnswersList);
    CUIXmlInit::InitScrollView(*m_uiXml, "answers_list", 0, UIAnswersList);
    UIAnswersList->SetWindowName("---UIAnswersList");

    //Вопросы
    UIQuestionsList = new CUIScrollView();
    UIQuestionsList->SetAutoDelete(true);
    //	UIDialogFrameBottom.AttachChild(UIQuestionsList);
    AttachChild(UIQuestionsList);
    CUIXmlInit::InitScrollView(*m_uiXml, "questions_list", 0, UIQuestionsList);
    UIQuestionsList->SetWindowName("---UIQuestionsList");

    //кнопка перехода в режим торговли
    AttachChild(&UIToTradeButton);
    CUIXmlInit::Init3tButton(*m_uiXml, "button", 0, &UIToTradeButton);

    // AttachChild					(&UIToExitButton);
    // CUIXmlInit::Init3tButton	(*m_uiXml, "button_exit", 0, &UIToExitButton);

    // m_btn_pos[0]				= UIToTradeButton.GetWndPos();
    // m_btn_pos[1]				= UIToExitButton.GetWndPos();
    // m_btn_pos[2].x				= (m_btn_pos[0].x+m_btn_pos[1].x)/2.0f;
    // m_btn_pos[2].y				= m_btn_pos[0].y;
    // шрифт для индикации имени персонажа в окне разговора
    CUIXmlInit::InitFont(*m_uiXml, "font", 0, m_iNameTextColor, m_pNameTextFont);

    CGameFont* pFont = NULL;
    CUIXmlInit::InitFont(*m_uiXml, "font", 1, m_uOurReplicsColor, pFont);

    SetWindowName("----CUITalkDialogWnd");

    Register(&UIToTradeButton);
    AddCallbackStr(
        "question_item", LIST_ITEM_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnQuestionClicked));
    AddCallback(
        &UIToTradeButton, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnTradeClicked));
    //	AddCallback					(&UIToExitButton,BUTTON_CLICKED,CUIWndCallback::void_function(this,
    //&CUITalkDialogWnd::OnExitClicked));
}

void CUITalkDialogWnd::Show()
{
    InventoryUtilities::SendInfoToActor("ui_talk_show");
    InventoryUtilities::SendInfoToLuaScripts("ui_talk_show");
    inherited::Show(true);
    inherited::Enable(true);

    ResetAll();
}

void CUITalkDialogWnd::Hide()
{
    InventoryUtilities::SendInfoToActor("ui_talk_hide");
    InventoryUtilities::SendInfoToLuaScripts("ui_talk_hide");
    inherited::Show(false);
    inherited::Enable(false);
    g_btnHint->Discard();
}

void CUITalkDialogWnd::OnQuestionClicked(CUIWindow* w, void*)
{
    m_ClickedQuestionID = ((CUIQuestionItem*)w)->m_s_value;
    GetMessageTarget()->SendMessage(this, TALK_DIALOG_QUESTION_CLICKED);
}

void CUITalkDialogWnd::OnExitClicked(CUIWindow* w, void*) { m_pParent->StopTalk(); }
void CUITalkDialogWnd::OnTradeClicked(CUIWindow* w, void*)
{
    if (mechanic_mode)
    {
        GetTop()->SendMessage(this, TALK_DIALOG_UPGRADE_BUTTON_CLICKED);
    }
    else
    {
        GetTop()->SendMessage(this, TALK_DIALOG_TRADE_BUTTON_CLICKED);
    }
}

void CUITalkDialogWnd::OnUpgradeClicked(CUIWindow* w, void*)
{
    GetTop()->SendMessage(this, TALK_DIALOG_UPGRADE_BUTTON_CLICKED);
}

void CUITalkDialogWnd::SetTradeMode() { OnTradeClicked(&UIToTradeButton, 0); }
//пересылаем сообщение родительскому окну для обработки
//и фильтруем если оно пришло от нашего дочернего окна
void CUITalkDialogWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }
void CUITalkDialogWnd::ClearAll()
{
    UIAnswersList->Clear();
    ClearQuestions();
}

void CUITalkDialogWnd::ClearQuestions() { UIQuestionsList->Clear(); }
void CUITalkDialogWnd::AddQuestion(LPCSTR str, LPCSTR value, int number, bool b_finalizer)
{
    CUIQuestionItem* itm = new CUIQuestionItem(m_uiXml, "question_item");
    itm->Init(value, str);
    ++number; // zero-based index
    if (number <= 10)
    {
        string16 buff;
        xr_sprintf(buff, "%d.", (number == 10) ? 0 : number);
        itm->m_num_text->SetText(buff);
        itm->m_text->SetAccelerator(SDL_SCANCODE_1 - 1 + number, 0);
    }
    if (b_finalizer)
    {
        itm->m_text->SetAccelerator(kQUIT, 2);
        itm->m_text->SetAccelerator(kUSE, 3);
    }

    itm->SetWindowName("question_item");
    UIQuestionsList->AddWindow(itm, true);
    Register(itm);
}

void CUITalkDialogWnd::AddAnswer(LPCSTR SpeakerName, LPCSTR str, bool bActor)
{
    CUIAnswerItem* itm = new CUIAnswerItem(m_uiXml, bActor ? "actor_answer_item" : "other_answer_item");
    itm->Init(str, SpeakerName);
    UIAnswersList->AddWindow(itm, true);
    UIAnswersList->ScrollToEnd();

    GAME_NEWS_DATA news_data;
    news_data.news_caption = SpeakerName;

    xr_string res;
    res = "%c[250,255,232,208]";
    res += str;
    news_data.news_text = res.c_str();

    news_data.m_type = GAME_NEWS_DATA::eTalk;
    CUICharacterInfo& ci = bActor ? UICharacterInfoLeft : UICharacterInfoRight;

    news_data.texture_name = ci.IconName();
    news_data.receive_time = Level().GetGameTime();

    Actor()->game_news_registry->registry().objects().push_back(news_data);
}

void CUITalkDialogWnd::AddIconedAnswer(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name)
{
    CUIAnswerItemIconed* itm = new CUIAnswerItemIconed(m_uiXml, templ_name);
    itm->Init(text, caption, texture_name);
    UIAnswersList->AddWindow(itm, true);
    UIAnswersList->ScrollToEnd();

    GAME_NEWS_DATA news_data;
    news_data.news_caption = caption;
    news_data.news_text._set(text);

    news_data.m_type = GAME_NEWS_DATA::eTalk;
    news_data.texture_name = texture_name;
    news_data.receive_time = Level().GetGameTime();

    Actor()->game_news_registry->registry().objects().push_back(news_data);
}

void CUITalkDialogWnd::SetOsoznanieMode(bool b)
{
    //	UIOurIcon.Show		(!b);
    //	UIOthersIcon.Show	(!b);

    UIAnswersList->Show(!b);
    //	UIDialogFrameTop.Show (!b);

    UIToTradeButton.Show(!b);
    if (mechanic_mode)
    {
        UIToTradeButton.m_hint_text = "ui_st_upgrade_hint";
        UIToTradeButton.TextItemControl()->SetTextST("ui_st_upgrade");
    }
    else
    {
        UIToTradeButton.m_hint_text = "ui_st_trade_hint";
        UIToTradeButton.TextItemControl()->SetTextST("ui_st_trade");
    }
}

void CUITalkDialogWnd::UpdateButtonsLayout(bool b_disable_break, bool trade_enabled)
{
    UIToTradeButton.Show(trade_enabled);
}

void CUIQuestionItem::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }
CUIQuestionItem::CUIQuestionItem(CUIXml* xml_doc, LPCSTR path)
{
    m_text = new CUI3tButton();
    m_text->SetAutoDelete(true);
    AttachChild(m_text);

    string512 str;
    CUIXmlInit xml_init;

    xr_strcpy(str, path);
    xml_init.InitWindow(*xml_doc, str, 0, this);

    m_min_height = xml_doc->ReadAttribFlt(path, 0, "min_height", 15.0f);

    strconcat(sizeof(str), str, path, ":content_text");
    xml_init.Init3tButton(*xml_doc, str, 0, m_text);

    Register(m_text);
    AddCallback(m_text, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIQuestionItem::OnTextClicked));

    m_num_text = new CUITextWnd();
    m_num_text->SetAutoDelete(true);
    AttachChild(m_num_text);
    strconcat(sizeof(str), str, path, ":num_text");
    xml_init.InitTextWnd(*xml_doc, str, 0, m_num_text);
}

void CUIQuestionItem::Init(LPCSTR val, LPCSTR text)
{
    m_s_value = val;
    m_text->TextItemControl()->SetText(text);
    m_text->AdjustHeightToText();
    float new_h = _max(m_min_height, m_text->GetWndPos().y + m_text->GetHeight());
    SetHeight(new_h);
}

void CUIQuestionItem::OnTextClicked(CUIWindow* w, void*)
{
    GetMessageTarget()->SendMessage(this, LIST_ITEM_CLICKED, (void*)this);
}

CUIAnswerItem::CUIAnswerItem(CUIXml* xml_doc, LPCSTR path)
{
    m_text = new CUITextWnd();
    m_text->SetAutoDelete(true);
    m_name = new CUITextWnd();
    m_name->SetAutoDelete(true);
    AttachChild(m_text);
    AttachChild(m_name);

    string512 str;
    CUIXmlInit xml_init;

    xr_strcpy(str, path);
    xml_init.InitWindow(*xml_doc, str, 0, this);

    m_min_height = xml_doc->ReadAttribFlt(path, 0, "min_height", 15.0f);
    m_bottom_footer = xml_doc->ReadAttribFlt(path, 0, "bottom_footer", 0.0f);
    strconcat(sizeof(str), str, path, ":content_text");
    xml_init.InitTextWnd(*xml_doc, str, 0, m_text);

    strconcat(sizeof(str), str, path, ":name_caption");
    xml_init.InitTextWnd(*xml_doc, str, 0, m_name);
    SetAutoDelete(true);
}

void CUIAnswerItem::Init(LPCSTR text, LPCSTR name)
{
    m_name->SetText(name);
    m_text->SetText(text);
    m_text->AdjustHeightToText();
    float new_h = _max(m_min_height, m_text->GetWndPos().y + m_text->GetHeight());
    new_h += m_bottom_footer;
    SetHeight(new_h);
}

CUIAnswerItemIconed::CUIAnswerItemIconed(CUIXml* xml_doc, LPCSTR path) : CUIAnswerItem(xml_doc, path)
{
    m_icon = new CUIStatic();
    m_icon->SetAutoDelete(true);
    AttachChild(m_icon);

    string512 str;
    CUIXmlInit xml_init;

    strconcat(sizeof(str), str, path, ":msg_icon");
    xml_init.InitStatic(*xml_doc, str, 0, m_icon);
}

void CUIAnswerItemIconed::Init(LPCSTR text, LPCSTR name, LPCSTR texture_name)
{
    xr_string res;
    res += name;
    res += "\\n %c[250,255,232,208]";
    res += text;

    inherited::Init(res.c_str(), "");
    m_icon->InitTexture(texture_name);
    m_icon->TextureOn();
    m_icon->SetStretchTexture(true);
}
