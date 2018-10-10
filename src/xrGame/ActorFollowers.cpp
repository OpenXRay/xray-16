#include "StdAfx.h"
/*
#include "ActorFollowers.h"
#include "ui/xrxmlparser.h"
#include "ui/UIFollowerPanel.h"
#include "Level.h"
#include "HUDManager.h"
#include "UIGameCustom.h"
#include "Actor.h"

CActorFollowerMngr::CActorFollowerMngr()
{
    CUIXml uiXml;
    uiXml.Init(CONFIG_PATH, UI_PATH, "follower_panel.xml");

    m_uiPanel = xr_new<CUIFollowerPanel>	();
    m_uiPanel->Init							(&uiXml,"followers_panel",0);
    HUD().GetUI()->UIGame()->AddDialogToRender(m_uiPanel);
    m_uiPanel->Show							(false);
}

CActorFollowerMngr::~CActorFollowerMngr()
{
    HUD().GetUI()->UIGame()->RemoveDialogToRender(m_uiPanel);
    xr_delete(m_uiPanel);
}

void CActorFollowerMngr::AddFollower(u16 id)
{
#ifdef DEBUG
    FOLLOWER_IT it = std::find(m_followers.begin(),m_followers.end(),id);
    if(it!=m_followers.end()){
        Msg("Attempt to add follower [%d] twice !!!",id);
        return;
    }
#endif

    m_followers.push_back(id);
    m_uiPanel->AddFollower(id);

}

void CActorFollowerMngr::RemoveFollower(u16 id)
{
    FOLLOWER_IT it = std::find(m_followers.begin(),m_followers.end(),id);
    if(it==m_followers.end()){
        Msg("Attempt to remove not registered follower [%d] !!!",id);
        return;
    }
    std::remove(m_followers.begin(),m_followers.end(),id);
    m_uiPanel->RemoveFollower(id);
}

void CActorFollowerMngr::SendCommand(int cmd)
{
    FOLLOWER_IT it		= m_followers.begin();
    FOLLOWER_IT it_e	= m_followers.end();
    CInventoryOwner* IO = NULL;
    for(;it!=it_e;++it){
        IO = smart_cast<CInventoryOwner*>(Level().Objects.net_Find(*it));
        IO->OnFollowerCmd				(cmd);
    }

}

CActorFollowerMngr& CActor::Followers()
{
    if(!m_followers)
        m_followers = xr_new<CActorFollowerMngr>();

    return *m_followers;
}

void CActor::AddFollower(u16 id)
{
    Followers().AddFollower			(id);
}

void CActor::RemoveFollower(u16 id)
{
    Followers().RemoveFollower			(id);
}

void CActor::DestroyFollowerInternal()
{
    xr_delete(m_followers);
}

void CActor::SendCmdToFollowers(int cmd)
{
    Followers().SendCommand(cmd);
}*/
