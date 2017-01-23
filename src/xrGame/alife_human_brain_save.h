/// DO NOT DELETE THIS FILE
/**
bool CSE_ALifeHumanAbstract::bfPerformAttack()
{
	if (!m_tpCurrentBestWeapon)
		return(false);

	switch (m_tpCurrentBestWeapon->m_dwSlot) {
		case 0 :
			return						(true);
		case 3 : {
			bool						l_bOk = false;
			OBJECT_IT			I = children.begin();
			OBJECT_IT			E = children.end();
			for ( ; I != E; ++I)
				if (*I == m_tpCurrentBestWeapon->ID) {
					l_bOk				= true;
					CSE_ALifeItem		*l_tpALifeItem = smart_cast<CSE_ALifeItem*>(ai().alife().objects().object(*I));
					alife().release		(l_tpALifeItem,true);
					break;
				}
			R_ASSERT2					(l_bOk,"Cannot find specified weapon in the inventory");
			return						(false);
		}
		default : {
			R_ASSERT2					(m_tpCurrentBestWeapon->m_dwAmmoAvailable,"No ammo for the selected weapon!");
			if (!m_trader_flags.test(eTraderFlagInfiniteAmmo))
				--(m_tpCurrentBestWeapon->m_dwAmmoAvailable);
			if (m_tpCurrentBestWeapon->m_dwAmmoAvailable)
				return					(true);

			for (int i=0, n=children.size() ; i<n; ++i) {
				CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(children[i]));
				if (l_tpALifeItemAmmo && strstr(m_tpCurrentBestWeapon->m_caAmmoSections,*l_tpALifeItemAmmo->s_name) && l_tpALifeItemAmmo->a_elapsed) {
					alife().release		(l_tpALifeItemAmmo,true);
					--i;
					--n;
				}
			}
			m_tpCurrentBestWeapon		= 0;
			return						(false);
		}
	}
}

EMeetActionType	CSE_ALifeHumanAbstract::tfGetActionType(CSE_ALifeSchedulable *tpALifeSchedulable, int iGroupIndex, bool bMutualDetection)
{
	if (eCombatTypeMonsterMonster == ai().alife().combat_type()) {
		CSE_ALifeMonsterAbstract	*l_tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract*>(tpALifeSchedulable);
		R_ASSERT2					(l_tpALifeMonsterAbstract,"Inconsistent meet action type");
		return						(eRelationTypeFriend == ai().alife().relation_type(this,smart_cast<CSE_ALifeMonsterAbstract*>(tpALifeSchedulable)) ? eMeetActionTypeInteract : ((bMutualDetection || (eCombatActionAttack == alife().choose_combat_action(iGroupIndex))) ? eMeetActionTypeAttack : eMeetActionTypeIgnore));
	}
	else
		return(eMeetActionTypeAttack);
}

void CSE_ALifeHumanAbstract::vfChooseGroup(CSE_ALifeGroupAbstract *tpALifeGroupAbstract)
{
	{
		OBJECT_IT					I = tpALifeGroupAbstract->m_tpMembers.begin();
		OBJECT_IT					E = tpALifeGroupAbstract->m_tpMembers.end();
		for ( ; I != E; ++I) {
			CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract = smart_cast<CSE_ALifeHumanAbstract*>(ai().alife().objects().object(*I));
			R_ASSERT2				(l_tpALifeHumanAbstract,"Invalid group member");
			l_tpALifeHumanAbstract->vfAttachItems(eTakeTypeMin);
		}
	}

	{
		OBJECT_IT					I = tpALifeGroupAbstract->m_tpMembers.begin();
		OBJECT_IT					E = tpALifeGroupAbstract->m_tpMembers.end();
		for ( ; I != E; ++I) {
			CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract = smart_cast<CSE_ALifeHumanAbstract*>(ai().alife().objects().object(*I));
			R_ASSERT2				(l_tpALifeHumanAbstract,"Invalid group member");
			l_tpALifeHumanAbstract->vfAttachItems(eTakeTypeRest);
		}
	}
}
/**/
