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
#include "UIHelper.h"

CUITalkDialogWnd::CUITalkDialogWnd()
    : CUIWindow("CUITalkDialogWnd"),
      m_uiXml(nullptr),
      m_pParent(nullptr),
      mechanic_mode(false),
      m_ClickedQuestionID(""),
      UIDialogFrameTop(nullptr),
      UIDialogFrameBottom(nullptr),
      m_btn_pos(),
      UIToExitButton(nullptr),
      UIOurIcon(nullptr),
      UIOthersIcon(nullptr),
      UIQuestionsList(nullptr),
      UIAnswersList(nullptr),
      m_pNameTextFont(nullptr),
      m_iNameTextColor(0),
      m_uOurReplicsColor(0) {}

CUITalkDialogWnd::~CUITalkDialogWnd() { xr_delete(m_uiXml); }

void CUITalkDialogWnd::InitTalkDialogWnd()
{
    static constexpr pcstr TALK_XML = "talk.xml";
    static constexpr pcstr TALK_CHARACTER_XML = "talk_character.xml";
    static constexpr cpcstr TRADE_CHARACTER_XML = "trade_character.xml";

    m_uiXml = xr_new<CUIXml>();
    m_uiXml->Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, TALK_XML);

    CUIXmlInit::InitWindow(*m_uiXml, "main", 0, this, false);

    UIStaticTop = UIHelper::CreateStatic(*m_uiXml, "top_background", this, false);
    UIStaticBottom = UIHelper::CreateStatic(*m_uiXml, "bottom_background", this, false);

    pcstr ourTag = "right_character_icon";
    pcstr othersTag = "left_character_icon";
    if (ShadowOfChernobylMode)
        std::swap(ourTag, othersTag);

    UIOurIcon = UIHelper::CreateStatic(*m_uiXml, ourTag, this, false);
    if (UIOurIcon)
    {
        UIOurIcon->AttachChild(&UICharacterInfoLeft);
        UICharacterInfoLeft.InitCharacterInfo(Fvector2().set(0, 0), UIOurIcon->GetWndSize(),
            TALK_CHARACTER_XML, TRADE_CHARACTER_XML);
    }

    UIOthersIcon = UIHelper::CreateStatic(*m_uiXml, othersTag, this, false);
    if (UIOthersIcon)
    {
        UIOthersIcon->AttachChild(&UICharacterInfoRight);
        UICharacterInfoRight.InitCharacterInfo(Fvector2().set(0, 0), UIOthersIcon->GetWndSize(),
            TALK_CHARACTER_XML, TRADE_CHARACTER_XML);
    }

    CUIWindow* answersParent = this;
    CUIWindow* questionsParent = this;

    // Our phrases frame
    UIDialogFrameBottom = UIHelper::CreateStatic(*m_uiXml, "frame_bottom", this, false);
    if (UIDialogFrameBottom)
        questionsParent = UIDialogFrameBottom;
    else
    {
        if (m_uiXml->NavigateToNode("frame_line_window", 1))
        {
            // XXX: Don't replace this with UI helper, until it's missing needed functionality to select the index
            UIOurPhrasesFrame = xr_new<CUIFrameLineWnd>("Our phrases frame");
            UIOurPhrasesFrame->SetAutoDelete(true);
            AttachChild(UIOurPhrasesFrame);
            CUIXmlInitBase::InitFrameLine(*m_uiXml, "frame_line_window", 1, UIOurPhrasesFrame); // index for field is 1 (one) !!!
            questionsParent = UIOurPhrasesFrame;
        }
    }

    // Main dialog frame
    UIDialogFrameTop = UIHelper::CreateStatic(*m_uiXml, "frame_top", this, false);
    if (UIDialogFrameTop)
        answersParent = UIDialogFrameTop;
    else
    {
        UIDialogFrame = UIHelper::CreateFrameLine(*m_uiXml, "frame_line_window", this, false);
        if (UIDialogFrame)
            answersParent = UIDialogFrame;
    }

    // Answers
    UIAnswersList = UIHelper::CreateScrollView(*m_uiXml, "answers_list", answersParent);
    UIAnswersList->SetWindowName("---UIAnswersList");

    // Questions
    UIQuestionsList = UIHelper::CreateScrollView(*m_uiXml, "questions_list", questionsParent);
    UIQuestionsList->SetWindowName("---UIQuestionsList");

    //кнопка перехода в режим торговли
    AttachChild(&UIToTradeButton);
    CUIXmlInit::Init3tButton(*m_uiXml, "button", 0, &UIToTradeButton);

    m_btn_pos[0] = UIToTradeButton.GetWndPos();

    UIToExitButton = UIHelper::Create3tButton(*m_uiXml, "button_exit", this, false);
    if (UIToExitButton)
    {
        m_btn_pos[1] = UIToExitButton->GetWndPos();
        m_btn_pos[2].x = (m_btn_pos[0].x + m_btn_pos[1].x) / 2.0f;
        m_btn_pos[2].y = m_btn_pos[0].y;
    }
    else
    {
        m_btn_pos[1] = m_btn_pos[0];
        m_btn_pos[2] = m_btn_pos[0];
    }

    // шрифт для индикации имени персонажа в окне разговора
    CUIXmlInit::InitFont(*m_uiXml, "font", 0, m_iNameTextColor, m_pNameTextFont);

    CGameFont* pFont = nullptr;
    CUIXmlInit::InitFont(*m_uiXml, "font", 1, m_uOurReplicsColor, pFont);

    Register(&UIToTradeButton);
    AddCallbackStr("question_item", LIST_ITEM_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnQuestionClicked));

    AddCallback(&UIToTradeButton, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnTradeClicked));

    //AddCallbackStr("upgrade_btn", BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnUpgradeClicked));

    if (UIToExitButton)
    {
        AddCallback(UIToExitButton, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUITalkDialogWnd::OnExitClicked));
    }
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
    CUIQuestionItem* itm = xr_new<CUIQuestionItem>(m_uiXml, "question_item");
    itm->Init(value, str);
    ++number; // zero-based index
    if (number <= 10)
    {
        string16 buff;
        xr_sprintf(buff, "%d.", (number == 10) ? 0 : number);
        if (itm->m_num_text)
            itm->m_num_text->SetText(buff);
        itm->m_text->SetAccelerator(SDL_SCANCODE_1 - 1 + number, 0);
    }
    if (b_finalizer)
    {
        itm->m_text->SetAccelerator(kQUIT, 2); // XXX: DON'T USE 2, instead use SDL_SCANCODE_*
        itm->m_text->SetAccelerator(kUSE, 3);
    }

    itm->SetWindowName("question_item");
    UIQuestionsList->AddWindow(itm, true);
    Register(itm);
}

void CUITalkDialogWnd::AddAnswer(LPCSTR SpeakerName, LPCSTR str, bool bActor)
{
    CUIAnswerItem* itm = xr_new<CUIAnswerItem>(m_uiXml, bActor ? "actor_answer_item" : "other_answer_item");
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
    CUIAnswerItemIconed* itm = xr_new<CUIAnswerItemIconed>(m_uiXml, templ_name);
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

void CUITalkDialogWnd::AddIconedAnswer(pcstr text, pcstr texture_name, Frect texture_rect, pcstr templ_name)
{
    CUIAnswerItemIconed* itm = xr_new<CUIAnswerItemIconed>(m_uiXml, templ_name);
    itm->Init(text, texture_name, texture_rect);
    UIAnswersList->AddWindow(itm, true);
    UIAnswersList->ScrollToEnd();

    GAME_NEWS_DATA news_data;
    news_data.news_caption = text;
    news_data.news_text = "";

    news_data.m_type = GAME_NEWS_DATA::eTalk;
    news_data.texture_name = texture_name;
    news_data.receive_time = Level().GetGameTime();

    Actor()->game_news_registry->registry().objects().push_back(news_data);
}

void CUITalkDialogWnd::SetOsoznanieMode(bool b)
{
    if (UIOurIcon)
        UIOurIcon->Show(!b);

    if (UIOthersIcon)
        UIOthersIcon->Show(!b);

    UIAnswersList->Show(!b);

    if (UIDialogFrameTop)
        UIDialogFrameTop->Show(!b);
    else if (UIDialogFrame)
        UIDialogFrame->Show(!b);

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

    if (UIToExitButton)
    {
        UIToExitButton->Show(!b_disable_break);

        if (UIToExitButton->IsShown() && UIToTradeButton.IsShown())
        {
            UIToTradeButton.SetWndPos(m_btn_pos[0]);
            UIToExitButton->SetWndPos(m_btn_pos[1]);
        }
        else if (UIToExitButton->IsShown())
        {
            UIToExitButton->SetWndPos(m_btn_pos[2]);
        }
        else if (UIToTradeButton.IsShown())
        {
            UIToTradeButton.SetWndPos(m_btn_pos[2]);
        }
    }
}

void CUITalkDialogWnd::TryScrollAnswersList(bool down)
{
    if (down)
        UIAnswersList->ScrollBar()->TryScrollDec();
    else
        UIAnswersList->ScrollBar()->TryScrollInc();
}

void CUIQuestionItem::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }
CUIQuestionItem::CUIQuestionItem(CUIXml* xml_doc, LPCSTR path)
    : CUIWindow("CUIQuestionItem")
{
    CUIXmlInit::InitWindow(*xml_doc, path, 0, this);

    m_min_height = xml_doc->ReadAttribFlt(path, 0, "min_height", 15.0f);

    string512 str;

    strconcat(sizeof(str), str, path, ":content_text");
    m_text = UIHelper::Create3tButton(*xml_doc, str, this);

    Register(m_text);
    AddCallback(m_text, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIQuestionItem::OnTextClicked));

    strconcat(sizeof(str), str, path, ":num_text");
    m_num_text = UIHelper::CreateStatic(*xml_doc, str, this, false);
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
    : CUIWindow("CUIAnswerItem")
{
    CUIXmlInit::InitWindow(*xml_doc, path, 0, this);

    m_min_height = xml_doc->ReadAttribFlt(path, 0, "min_height", 15.0f);
    m_bottom_footer = xml_doc->ReadAttribFlt(path, 0, "bottom_footer", 0.0f);

    string512 str;

    strconcat(sizeof(str), str, path, ":content_text");
    m_text = UIHelper::CreateStatic(*xml_doc, str, this);

    strconcat(sizeof(str), str, path, ":name_caption");
    m_name = UIHelper::CreateStatic(*xml_doc, str, this);

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
    m_icon = xr_new<CUIStatic>("Icon");
    m_icon->SetAutoDelete(true);
    CUIWindow::AttachChild(m_icon);

    string512 str;
    strconcat(sizeof(str), str, path, ":msg_icon");

    CUIXmlInit::InitStatic(*xml_doc, str, 0, m_icon);
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

void CUIAnswerItemIconed::Init(pcstr text, pcstr texture_name, Frect texture_rect)
{
    inherited::Init(text, "");
    if (texture_name && texture_name[0])
        m_icon->InitTexture(texture_name);
    m_icon->SetTextureRect(texture_rect);
    m_icon->TextureOn();
    m_icon->SetStretchTexture(true);
}
