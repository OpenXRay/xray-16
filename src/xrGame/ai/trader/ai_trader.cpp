////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_trader.cpp
//	Created 	: 13.05.2002
//  Modified 	: 13.05.2002
//	Author		: Jim
//	Description : AI Behaviour for monster "Trader"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_trader.h"
#include "trade.h"
#include "script_entity_action.h"
#include "script_game_object.h"
#include "Inventory.h"
#include "xrServerEntities/xrServer_Objects_ALife_Monsters.h"
#include "Artefact.h"
#include "xrServer.h"
#include "relation_registry.h"
#include "Common/object_broker.h"
#include "sound_player.h"
#include "Level.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "game_object_space.h"
#include "trader_animation.h"
#include "xrServerEntities/clsid_game.h"

CAI_Trader::CAI_Trader() { AnimMan = new CTraderAnimation(this); }
CAI_Trader::~CAI_Trader()
{
    xr_delete(m_sound_player);
    xr_delete(AnimMan);
}

void CAI_Trader::Load(LPCSTR section)
{
    //	setEnabled						(FALSE);
    inherited::Load(section);

    // fHealth							= pSettings->r_float	(section,"Health");
    SetfHealth(pSettings->r_float(section, "Health"));

    float max_weight = pSettings->r_float(section, "max_item_mass");
    inventory().SetMaxWeight(max_weight * 1000);
    //	inventory().SetMaxRuck(1000000);
    inventory().CalcTotalWeight();
}

void CAI_Trader::reinit()
{
    CScriptEntity::reinit();
    CEntityAlive::reinit();
    CInventoryOwner::reinit();
    sound().reinit();
    animation().reinit();

    m_busy_now = false;
}

void CAI_Trader::reload(LPCSTR section)
{
    CEntityAlive::reload(section);
    CInventoryOwner::reload(section);
    sound().reload(section);
}

bool CAI_Trader::bfAssignSound(CScriptEntityAction* tpEntityAction)
{
    if (!CScriptEntity::bfAssignSound(tpEntityAction))
    {
        // m_cur_head_anim_type	= MonsterSpace::eHeadAnimNone;
        return (false);
    }

    // CScriptSoundAction	&l_tAction	= tpEntityAction->m_tSoundAction;
    // m_cur_head_anim_type = l_tAction.m_tHeadAnimType;

    return (true);
}

//////////////////////////////////////////////////////////////////////////
// Look At Actor
//////////////////////////////////////////////////////////////////////////
void CAI_Trader::BoneCallback(CBoneInstance* B)
{
    CAI_Trader* this_class = static_cast<CAI_Trader*>(B->callback_param());

    this_class->LookAtActor(B);
    R_ASSERT2(_valid(B->mTransform), "CAI_Trader::BoneCallback");
}

void CAI_Trader::LookAtActor(CBoneInstance* B)
{
    Fvector dir;
    dir.sub(Level().CurrentEntity()->Position(), Position());

    float yaw, pitch;
    dir.getHP(yaw, pitch);

    float h, p, b;
    XFORM().getHPB(h, p, b);
    float cur_yaw = h;
    float dy = _abs(angle_normalize_signed(yaw - cur_yaw));

    if (angle_normalize_signed(yaw - cur_yaw) > 0)
        dy *= -1.f;

    Fmatrix M;
    M.setHPB(0.f, -dy, 0.f);
    B->mTransform.mulB_43(M);
}

//////////////////////////////////////////////////////////////////////////

BOOL CAI_Trader::net_Spawn(CSE_Abstract* DC)
{
    CSE_Abstract* e = (CSE_Abstract*)(DC);
    CSE_ALifeTrader* l_tpTrader = smart_cast<CSE_ALifeTrader*>(e);
    R_ASSERT(l_tpTrader);

    //проспавнить PDA у InventoryOwner
    if (!CInventoryOwner::net_Spawn(DC))
        return (FALSE);

    if (!inherited::net_Spawn(DC) || !CScriptEntity::net_Spawn(DC))
        return (FALSE);

    setVisible(TRUE);
    setEnabled(TRUE);

    set_money(l_tpTrader->m_dwMoney, false);

    // Установка callback на кости
    CBoneInstance* bone_head = &smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(
        smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_head"));
    bone_head->set_callback(bctCustom, BoneCallback, this);

    shedule.t_min = 100;
    shedule.t_max = 2500; // This equaltiy is broken by Dima :-( // 30 * NET_Latency / 4;

    return (TRUE);
}

void CAI_Trader::net_Export(NET_Packet& P)
{
    R_ASSERT(Local());

    //	P.w_float						(inventory().TotalWeight());
    //	P.w_u32							(m_dwMoney);
}

void CAI_Trader::net_Import(NET_Packet& P)
{
    R_ASSERT(Remote());

    float fDummy;
    P.r_float(fDummy);
    set_money(P.r_u32(), false);

    setVisible(TRUE);
    setEnabled(TRUE);
}

void CAI_Trader::OnEvent(NET_Packet& P, u16 type)
{
    inherited::OnEvent(P, type);
    CInventoryOwner::OnEvent(P, type);

    u16 id;
    IGameObject* Obj;

    switch (type)
    {
    case GE_TRADE_BUY:
    case GE_OWNERSHIP_TAKE:
        P.r_u16(id);
        Obj = Level().Objects.net_Find(id);
        if (inventory().CanTakeItem(smart_cast<CInventoryItem*>(Obj)))
        {
            Obj->H_SetParent(this);
            inventory().Take(smart_cast<CGameObject*>(Obj), false, false);
        }
        else
        {
            NET_Packet P;
            u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
            P.w_u16(u16(Obj->ID()));
            u_EventSend(P);
        }
        break;
    case GE_TRADE_SELL:
    case GE_OWNERSHIP_REJECT:
    {
        P.r_u16(id);
        Obj = Level().Objects.net_Find(id);
        bool just_before_destroy = !P.r_eof() && P.r_u8();
        bool dont_create_shell = (type == GE_TRADE_SELL) || just_before_destroy;

        Obj->SetTmpPreDestroy(just_before_destroy);
        inventory().DropItem(smart_cast<CGameObject*>(Obj), just_before_destroy, dont_create_shell);
        // if(inventory().DropItem(smart_cast<CGameObject*>(Obj), just_before_destroy))
        //	Obj->H_SetParent(0, just_before_destroy); //moved to DropItem
    }
    break;
    case GE_TRANSFER_AMMO: break;
    }
}

void CAI_Trader::feel_touch_new(IGameObject* O)
{
    if (!g_Alive())
        return;
    if (Remote())
        return;

    // Now, test for game specific logical objects to minimize traffic
    CInventoryItem* I = smart_cast<CInventoryItem*>(O);

    if (I && I->useful_for_NPC())
    {
        Msg("Taking item %s!", *I->object().cName());
        NET_Packet P;
        u_EventGen(P, GE_OWNERSHIP_TAKE, ID());
        P.w_u16(u16(I->object().ID()));
        u_EventSend(P);
    }
}

void CAI_Trader::DropItemSendMessage(IGameObject* O)
{
    if (!O || !O->H_Parent() || (this != O->H_Parent()))
        return;

    Msg("Dropping item!");
    // We doesn't have similar weapon - pick up it
    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
    P.w_u16(u16(O->ID()));
    u_EventSend(P);
}

void CAI_Trader::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);
    UpdateInventoryOwner(dt);

    if (GetScriptControl())
        ProcessScripts();
    else
        Think();
}

void CAI_Trader::g_WeaponBones(int& L, int& R1, int& R2)
{
    IKinematics* V = smart_cast<IKinematics*>(Visual());
    R1 = V->LL_BoneID("bip01_r_hand");
    R2 = V->LL_BoneID("bip01_r_finger2");
    L = V->LL_BoneID("bip01_l_finger1");
}

void CAI_Trader::g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D)
{
    VERIFY(inventory().ActiveItem());
    if (g_Alive() && inventory().ActiveItem())
    {
        Center(P);
        D.setHP(0, 0);
        D.normalize_safe();
    }
}

void CAI_Trader::Think() {}
void CAI_Trader::Die(IGameObject* who) { inherited::Die(who); }
void CAI_Trader::net_Destroy()
{
    inherited::net_Destroy();
    CScriptEntity::net_Destroy();
}

void CAI_Trader::UpdateCL()
{
    inherited::UpdateCL();
    sound().update(Device.fTimeDelta);

    if (!GetScriptControl() && !bfScriptAnimation())
        animation().update_frame();
}

BOOL CAI_Trader::UsedAI_Locations() { return (TRUE); }
void CAI_Trader::OnStartTrade()
{
    m_busy_now = true;
    callback(GameObject::eTradeStart)();
}

void CAI_Trader::OnStopTrade()
{
    m_busy_now = false;
    callback(GameObject::eTradeStop)();
}

////////////////////////////////////////////////////////////////////////////////////////////

bool CAI_Trader::can_attach(const CInventoryItem* inventory_item) const { return (false); }
bool CAI_Trader::use_bolts() const { return (false); }
void CAI_Trader::spawn_supplies()
{
    inherited::spawn_supplies();
    CInventoryOwner::spawn_supplies();
}

void CAI_Trader::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);
    CInventoryOwner::save(output_packet);
}
void CAI_Trader::load(IReader& input_packet)
{
    inherited::load(input_packet);
    CInventoryOwner::load(input_packet);
}

//проверяет список артефактов в заказах
u32 CAI_Trader::ArtefactPrice(CArtefact* pArtefact) { return pArtefact->Cost(); }
//продажа артефакта, с последуещим изменением списка заказов (true - если артефакт был в списке)
bool CAI_Trader::BuyArtefact(CArtefact* pArtefact)
{
    VERIFY(pArtefact);
    return false;
}

ALife::ERelationType CAI_Trader::tfGetRelationType(const CEntityAlive* tpEntityAlive) const
{
    const CInventoryOwner* pOtherIO = smart_cast<const CInventoryOwner*>(tpEntityAlive);

    ALife::ERelationType relation = ALife::eRelationTypeDummy;

    if (pOtherIO && !(const_cast<CEntityAlive*>(tpEntityAlive)->cast_base_monster()))
        relation = RELATION_REGISTRY().GetRelationType(static_cast<const CInventoryOwner*>(this), pOtherIO);

    if (ALife::eRelationTypeDummy != relation)
        return relation;
    else
        return inherited::tfGetRelationType(tpEntityAlive);
}

IFactoryObject* CAI_Trader::_construct()
{
    m_sound_player = new CSoundPlayer(this);

    CEntityAlive::_construct();
    CInventoryOwner::_construct();
    CScriptEntity::_construct();

    return (this);
}

bool CAI_Trader::AllowItemToTrade(CInventoryItem const* item, const SInvItemPlace& place) const
{
    if (!g_Alive())
        return (true);

    if (item->object().CLS_ID == CLSID_DEVICE_PDA)
        return (false);

    return (CInventoryOwner::AllowItemToTrade(item, place));
}

void CAI_Trader::dialog_sound_start(LPCSTR phrase) { animation().external_sound_start(phrase); }
void CAI_Trader::dialog_sound_stop() { animation().external_sound_stop(); }
