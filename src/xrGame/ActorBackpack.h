#pragma once
#include "inventory_item_object.h"

class CBackpack final : public CInventoryItemObject
{
	using inherited = CInventoryItemObject;

public:
	CBackpack();

	void Load(pcstr section) override;

	void Hit(float P, ALife::EHitType hit_type) override;

	void OnMoveToSlot(const SInvItemPlace& prev) override;
	void OnMoveToRuck(const SInvItemPlace& previous_place) override;
	void OnH_A_Chield() override;

public:
	float m_additional_weight;
	float m_additional_weight2;
	float m_fPowerRestoreSpeed;
	float m_fPowerLoss;

    float m_fJumpSpeed;
    float m_fWalkAccel;
    float m_fOverweightWalkK;

	bool net_Spawn(CSE_Abstract* DC) override;
	void net_Export(NET_Packet& P) override;
	void net_Import(NET_Packet& P) override;

protected:
    bool install_upgrade_impl(pcstr section, bool test) override;
};
