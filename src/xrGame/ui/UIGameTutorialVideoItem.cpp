#include "pch_script.h"
#include "UIGameTutorial.h"
#include "UIWindow.h"
#include "UIStatic.h"
#include "UIXmlInit.h"
#include "Common/object_broker.h"
#include "xrEngine/xr_input.h"
#include "xr_level_controller.h"

#include "Include/xrRender/UISequenceVideoItem.h"
#include "Include/xrRender/UIShader.h"
#include "Include/xrRender/UIRender.h"
#include "uicursor.h"

extern ENGINE_API BOOL bShowPauseString;

//-----------------------------------------------------------------------------
// Tutorial Item
//-----------------------------------------------------------------------------
CUISequenceVideoItem::CUISequenceVideoItem(CUISequencer* owner) : CUISequenceItem(owner)
{
    // m_texture				= NULL;
    m_flags.set(etiPlaying | etiNeedStart | etiDelayed | etiBackVisible, FALSE);
    m_delay = 0.f;
    m_wnd = NULL;
    m_wnd_bg = NULL;
    m_delay = 0.f;
    m_time_start = 0;
    m_sync_time = 0;
}

CUISequenceVideoItem::~CUISequenceVideoItem()
{
    m_sound.stop();
    delete_data(m_wnd);
    delete_data(m_wnd_bg);
}

bool CUISequenceVideoItem::IsPlaying() { return (!!m_flags.test(etiPlaying)); }
void CUISequenceVideoItem::Load(CUIXml* xml, int idx)
{
    CUISequenceItem::Load(xml, idx);

    XML_NODE _stored_root = xml->GetLocalRoot();
    xml->SetLocalRoot(xml->NavigateToNode("item", idx));

    LPCSTR str = xml->Read("pause_state", 0, "ignore");
    m_flags.set(etiNeedPauseOn, 0 == xr_stricmp(str, "on"));
    m_flags.set(etiNeedPauseOff, 0 == xr_stricmp(str, "off"));
    m_flags.set(etiNeedPauseSound, 0 == xr_stricmp(str, "on"));

    str = xml->Read("can_be_stopped", 0, "on");
    m_flags.set(etiCanBeStopped, 0 == xr_stricmp(str, "on"));

    str = xml->Read("back_show", 0, "on");
    m_flags.set(etiBackVisible, 0 == xr_stricmp(str, "on"));

    m_flags.set(etiGrabInput, TRUE);

    m_delay = _max(xml->ReadFlt("delay", 0, 0.f), 0.f);

    // ui-components
    if (xml->NavigateToNode("background", 0))
    {
        m_wnd_bg = new CUIStatic();
        m_wnd_bg->SetAutoDelete(false);
        CUIXmlInit::InitStatic(*xml, "background", 0, m_wnd_bg);
    }
    m_wnd = new CUIStatic();
    m_wnd->SetAutoDelete(false);
    CUIXmlInit::InitStatic(*xml, "video_wnd", 0, m_wnd);
    bool bFullScreen = (1 == xml->ReadAttribInt("video_wnd", 0, "fullscreen", 0));
    if (!bFullScreen)
    {
        m_wnd->SetWndPos(Fvector2().set(512.0f, 384.0f));
        m_wnd->SetAlignment(waCenter);
        Frect texture_coords = m_wnd->GetUIStaticItem().GetTextureRect();

        bool is_16_9 = UI().is_widescreen();
        float kw_image = UI_BASE_WIDTH / texture_coords.width();

        Fvector2 wnd_size;

        wnd_size.x = UI_BASE_WIDTH;
        wnd_size.y = texture_coords.height() * kw_image;
        if (is_16_9)
            wnd_size.y *= 1.2f;

        m_wnd->SetWndSize(wnd_size);
    }
    LPCSTR snd_name = xml->Read("sound", 0, "");

    if (snd_name && snd_name[0])
    {
        m_sound.create(snd_name, st_Effect, sg_Undefined);
        VERIFY(m_sound._handle() || strstr(Core.Params, "-nosound"));
    }
    xml->SetLocalRoot(_stored_root);
}

void CUISequenceVideoItem::Update()
{
    inherited::Update();
    if (GetUICursor().IsVisible())
    {
        m_flags.set(etiStoredCursorState, TRUE);
        GetUICursor().Hide();
    }
    // deferred start
    if (Device.dwTimeContinual >= m_time_start)
    {
        if (m_flags.test(etiDelayed))
        {
            if (m_wnd_bg)
            {
                m_owner->MainWnd()->AttachChild(m_wnd_bg);
                m_wnd_bg->Show(true);
            }
            m_owner->MainWnd()->AttachChild(m_wnd);
            m_wnd->Show(true);
            m_flags.set(etiDelayed, FALSE);
        }
    }
    else
        return;

    u32 sync_tm = (0 == m_sound._handle()) ? Device.dwTimeContinual :
                                             (m_sound._feedback() ? m_sound._feedback()->play_time() : m_sync_time);
    m_sync_time = sync_tm;
    // processing A&V

    if (m_texture->HasTexture())
    {
        BOOL is_playing = m_sound._handle() ? !!m_sound._feedback() : m_texture->video_IsPlaying();
        if (is_playing)
        {
            m_texture->video_Sync(m_sync_time);
        }
        else
        {
            // sync start
            if (m_flags.test(etiNeedStart))
            {
                m_sound.play_at_pos(NULL, Fvector().set(0.0f, 0.f, 0.0f), sm_2D);
                m_texture->video_Play(FALSE, m_sync_time);
                m_flags.set(etiNeedStart, FALSE);
                CUIWindow* w = m_owner->MainWnd()->FindChild("back");
                if (w)
                    w->Show(!!m_flags.test(etiBackVisible));
            }
            else
            {
                m_flags.set(etiPlaying, FALSE);
            }
        }
    }
}

void CUISequenceVideoItem::OnRender()
{
    if (!m_texture->HasTexture() && m_wnd->GetShader() && m_wnd->GetShader()->inited())
    {
        GEnv.UIRender->SetShader(*m_wnd->GetShader());
        m_texture->CaptureTexture();
        m_texture->video_Stop();
    }
}

void CUISequenceVideoItem::Start()
{
    inherited::Start();
    m_flags.set(etiStoredPauseState, Device.Paused());

    if (m_flags.test(etiNeedPauseOn) && !m_flags.test(etiStoredPauseState))
    {
        Device.Pause(TRUE, TRUE, TRUE, "videoitem_start");
        bShowPauseString = FALSE;
    }

    if (m_flags.test(etiNeedPauseOff) && m_flags.test(etiStoredPauseState))
        Device.Pause(FALSE, TRUE, TRUE, "videoitem_start");

    if (m_flags.test(etiNeedPauseSound))
        Device.Pause(TRUE, FALSE, TRUE, "videoitem_start");

    m_flags.set(etiPlaying, TRUE);
    m_flags.set(etiNeedStart, TRUE);

    m_sync_time = 0;
    m_time_start = Device.dwTimeContinual + iFloor(m_delay * 1000.f);
    m_flags.set(etiDelayed, TRUE);

    if (m_flags.test(etiBackVisible))
    {
        CUIWindow* w = m_owner->MainWnd()->FindChild("back");
        if (w)
            w->Show(true);
    }
}

bool CUISequenceVideoItem::Stop(bool bForce)
{
    if (m_flags.test(etiStoredCursorState))
        GetUICursor().Show();

    if (!m_flags.test(etiCanBeStopped) && !bForce && IsPlaying())
        return false;

    m_flags.set(etiPlaying, FALSE);

    m_wnd->Show(false);
    if (Device.dwTimeContinual >= m_time_start && m_wnd->GetParent() == m_owner->MainWnd())
        m_owner->MainWnd()->DetachChild(m_wnd);

    m_sound.stop();
    m_texture->ResetTexture();

    if (m_flags.test(etiNeedPauseOn) && !m_flags.test(etiStoredPauseState))
        Device.Pause(FALSE, TRUE, TRUE, "videoitem_stop");

    if (m_flags.test(etiNeedPauseOff) && m_flags.test(etiStoredPauseState))
        Device.Pause(TRUE, TRUE, TRUE, "videoitem_stop");

    if (m_flags.test(etiNeedPauseSound))
        Device.Pause(FALSE, FALSE, TRUE, "videoitem_stop");

    inherited::Stop();
    return true;
}
