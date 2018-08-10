// UICharacterInfo.h:  окошко, с информацией о персонаже
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "xrUICore/Windows/UIWindow.h"
#include "xrServerEntities/alife_space.h"
#include "character_info_defs.h"

class CUIStatic;
class CCharacterInfo;
class CUIXml;
class CUIScrollView;
class CUICharacterInfo : public CUIWindow
{
private:
    typedef CUIWindow inherited;

protected:
    void SetRelation(ALife::ERelationType relation, CHARACTER_GOODWILL goodwill);
    void ResetAllStrings();
    void UpdateRelation();
    bool hasOwner() { return (m_ownerID != u16(-1)); }
    // Biography
    CUIScrollView* pUIBio;
    bool m_bForceUpdate;
    u16 m_ownerID;

    enum UIItemType
    {
        eIcon = 0,
        eIconOver,
        /*
                eRankIcon,
                eRankIconOver,
                eCommunityIcon,
                eCommunityIconOver,
                eCommunityBigIcon,
                eCommunityBigIconOver,
        */
        eName,
        eNameCaption,
        eRank,
        eRankCaption,
        eCommunity,
        eCommunityCaption,
        eReputation,
        eReputationCaption,
        eRelation,
        eRelationCaption,

        eMaxCaption
    };
    CUIStatic* m_icons[eMaxCaption];
    shared_str m_texture_name;
    u32 m_deadbody_color;

public:
    CUICharacterInfo();
    virtual ~CUICharacterInfo();

    void InitCharacterInfo(Fvector2 pos, Fvector2 size, CUIXml* xml_doc);
    void InitCharacterInfo(Fvector2 pos, Fvector2 size, LPCSTR xml_name);
    void InitCharacterInfo(CUIXml* xml_doc, LPCSTR node_str);
    void Init_StrInfoItem(CUIXml& xml_doc, LPCSTR item_str, UIItemType type);
    void Init_IconInfoItem(CUIXml& xml_doc, LPCSTR item_str, UIItemType type);

    void InitCharacter(u16 id);
    void ClearInfo();
    void InitCharacterMP(LPCSTR player_name, LPCSTR player_icon);

    virtual void Update();

    u16 OwnerID() const { return m_ownerID; }
    CUIStatic& UIIcon() const
    {
        VERIFY(m_icons[eIcon]);
        return *m_icons[eIcon];
    }
    CUIStatic& UIName() const
    {
        VERIFY(m_icons[eName]);
        return *m_icons[eName];
    }
    CUIStatic& UICommunity() const
    {
        VERIFY(m_icons[eCommunity]);
        return *m_icons[eCommunity];
    }
    CUIStatic& UICommunityCaption() const
    {
        VERIFY(m_icons[eCommunityCaption]);
        return *m_icons[eCommunityCaption];
    }

    const shared_str& IconName() const { return m_texture_name; }
    static bool get_actor_community(shared_str* our, shared_str* enemy);
    static bool ignore_community(shared_str const& check_community);
};
