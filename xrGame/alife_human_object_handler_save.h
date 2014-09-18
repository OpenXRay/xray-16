/// DO NOT DELETE THIS FILE


/**
struct CSortItemPredicate {
	IC bool							operator()							(const CSE_ALifeInventoryItem *tpALifeInventoryItem1, const CSE_ALifeInventoryItem *tpALifeInventoryItem2)  const
	{
		return						(float(tpALifeInventoryItem1->m_dwCost) > float(tpALifeInventoryItem2->m_dwCost));
	};
};

struct CSortItemVolumePredicate {
	IC bool							operator()							(const CSE_ALifeInventoryItem *tpALifeInventoryItem1, const CSE_ALifeInventoryItem *tpALifeInventoryItem2)  const
	{
		return						(float(tpALifeInventoryItem1->m_iGridWidth*tpALifeInventoryItem1->m_iGridHeight) > float(tpALifeInventoryItem2->m_iGridWidth*tpALifeInventoryItem2->m_iGridHeight));
	};
};

struct CRemoveSlotAndCellItemsPredicate {
	u32								m_dwMaxCount;
	u32								m_dwCurCount;
	ALife::WEAPON_P_VECTOR			*m_temp_weapons;

	CRemoveSlotAndCellItemsPredicate	(ALife::WEAPON_P_VECTOR *tpWeaponVector, u32 dwMaxCount) : m_dwMaxCount(dwMaxCount), m_dwCurCount(0)
	{
		m_temp_weapons			= tpWeaponVector;
	}

	IC bool							operator()							(const CSE_ALifeInventoryItem *tpALifeInventoryItem)
	{
		const CSE_ALifeItemWeapon	*l_tpALifeItemWeapon = smart_cast<const CSE_ALifeItemWeapon*>(tpALifeInventoryItem);
		if (l_tpALifeItemWeapon && ((*m_temp_weapons)[l_tpALifeItemWeapon->m_dwSlot] == l_tpALifeItemWeapon))
			return					(true);
		else
			if ((m_dwCurCount < m_dwMaxCount) && (1 == tpALifeInventoryItem->m_iVolume)) {
				++m_dwCurCount;
				return				(true);
			}
		return						(false);
	}
};

CSE_ALifeItemWeapon	*CSE_ALifeHumanAbstract::tpfGetBestWeapon(EHitType &tHitType, float &fHitPower)
{
	fHitPower					= 0.f;
	m_tpCurrentBestWeapon		= 0;
	u32							l_dwBestWeapon = 0;
	OBJECT_IT			I = children.begin();
	OBJECT_IT			E = children.end();
	for ( ; I != E; ++I) {
		CSE_ALifeItemWeapon		*l_tpALifeItemWeapon = smart_cast<CSE_ALifeItemWeapon*>(ai().alife().objects().object(*I));
		if (!l_tpALifeItemWeapon)
			continue;

		l_tpALifeItemWeapon->m_dwAmmoAvailable = get_available_ammo_count(l_tpALifeItemWeapon,children);
		if (l_tpALifeItemWeapon->m_dwAmmoAvailable || (!l_tpALifeItemWeapon->m_dwSlot) || (3 == l_tpALifeItemWeapon->m_dwSlot)) {
			u32					l_dwCurrentBestWeapon = l_tpALifeItemWeapon->ef_weapon_type(); 
			if (l_dwCurrentBestWeapon > l_dwBestWeapon) {
				l_dwBestWeapon = l_dwCurrentBestWeapon;
				m_tpCurrentBestWeapon = l_tpALifeItemWeapon;
			}
		}
	}
	if (m_tpCurrentBestWeapon) {
		fHitPower				= m_tpCurrentBestWeapon->m_fHitPower;
		tHitType				= m_tpCurrentBestWeapon->m_tHitType;
		return					(m_tpCurrentBestWeapon);
	}
	else
		return(inherited2::tpfGetBestWeapon(tHitType,fHitPower));
}

void CSE_ALifeHumanAbstract::vfCollectAmmoBoxes()
{
	for (int i=0, n=children.size() ; i<n; ++i) {
		
		if (alife().m_temp_marks[i])
			continue;
		
		alife().m_temp_marks[i]	= true;
		
		CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(children[i]));
		if (!l_tpALifeItemAmmo)
			continue;

		for (int j=i+1; j<n; ++j) {
			if (alife().m_temp_marks[j])
				continue;

			CSE_ALifeItemAmmo	*l_tpALifeItemAmmo1 = smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(children[j]));
			if (!l_tpALifeItemAmmo1) {
				alife().m_temp_marks[j]	= true;
				continue;
			}

			if (!strstr(*l_tpALifeItemAmmo->s_name,*l_tpALifeItemAmmo1->s_name))
				continue;

			alife().m_temp_marks[j]	= true;

			if (l_tpALifeItemAmmo->a_elapsed + l_tpALifeItemAmmo1->a_elapsed > l_tpALifeItemAmmo->m_boxSize) {
				l_tpALifeItemAmmo1->a_elapsed	= l_tpALifeItemAmmo->a_elapsed + l_tpALifeItemAmmo1->a_elapsed - l_tpALifeItemAmmo->m_boxSize;
				l_tpALifeItemAmmo->a_elapsed	= l_tpALifeItemAmmo->m_boxSize;
				l_tpALifeItemAmmo				= l_tpALifeItemAmmo1;
			}
			else {
				l_tpALifeItemAmmo->a_elapsed	= l_tpALifeItemAmmo->a_elapsed + l_tpALifeItemAmmo1->a_elapsed;
				l_tpALifeItemAmmo1->a_elapsed	= 0;
			}
		}
	}

	for (int i=0, j=0; i<n; ++i,++j) {
		alife().m_temp_marks[j] = false;

		CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(children[i]));
		if (!l_tpALifeItemAmmo || l_tpALifeItemAmmo->a_elapsed)
			continue;

		alife().release			(l_tpALifeItemAmmo,true);
		--i;
		--n;
	}
}

void CSE_ALifeHumanAbstract::vfUpdateWeaponAmmo()
{
	if (!m_tpCurrentBestWeapon)
		return;

	switch (m_tpCurrentBestWeapon->m_dwSlot) {
		case 0 :
		case 3 :
			break;
		default : {
			for (int i=0, n=children.size() ; i<n; ++i) {
				CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(children[i]));
				if (l_tpALifeItemAmmo && strstr(m_tpCurrentBestWeapon->m_caAmmoSections,*l_tpALifeItemAmmo->s_name)) {
					if (m_tpCurrentBestWeapon->m_dwAmmoAvailable > l_tpALifeItemAmmo->a_elapsed) {
						m_tpCurrentBestWeapon->m_dwAmmoAvailable	-= l_tpALifeItemAmmo->a_elapsed;
						continue;
					}
					if (m_tpCurrentBestWeapon->m_dwAmmoAvailable) {
						l_tpALifeItemAmmo->a_elapsed = (u16)m_tpCurrentBestWeapon->m_dwAmmoAvailable;
						m_tpCurrentBestWeapon->m_dwAmmoAvailable = 0;
						continue;
					}
					alife().release			(l_tpALifeItemAmmo,true);
					--i;
					--n;
				}
			}
			m_tpCurrentBestWeapon = 0;
			break;
		}
	}
	vfCollectAmmoBoxes();
}

u16	CSE_ALifeHumanAbstract::get_available_ammo_count(const CSE_ALifeItemWeapon *tpALifeItemWeapon, OBJECT_VECTOR &tpObjectVector)
{
	if (!tpALifeItemWeapon->m_caAmmoSections)
		return(u16(-1));
	u32							l_dwResult = 0;
	OBJECT_IT					I = tpObjectVector.begin();
	OBJECT_IT					E = tpObjectVector.end();
	for ( ; I != E; ++I) {
		CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(ai().alife().objects().object(*I));
		if (l_tpALifeItemAmmo && strstr(tpALifeItemWeapon->m_caAmmoSections,*l_tpALifeItemAmmo->s_name))
			l_dwResult			+= l_tpALifeItemAmmo->a_elapsed;
	}
	return						(u16(l_dwResult));
}

u16	CSE_ALifeHumanAbstract::get_available_ammo_count(const CSE_ALifeItemWeapon *tpALifeItemWeapon, ITEM_P_VECTOR &tpItemVector, OBJECT_VECTOR *tpObjectVector)
{
	if (!tpALifeItemWeapon->m_caAmmoSections)
		return(u16(-1));
	u32							l_dwResult = 0;
	ITEM_P_IT					I = tpItemVector.begin();
	ITEM_P_IT					E = tpItemVector.end();
	for ( ; I != E; ++I) {
		CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(*I);
		if (l_tpALifeItemAmmo && strstr(tpALifeItemWeapon->m_caAmmoSections,*l_tpALifeItemAmmo->s_name) && (l_tpALifeItemAmmo->m_dwCost <= m_dwTotalMoney) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),l_tpALifeItemAmmo->ID) == tpObjectVector->end()))) {
			l_dwResult			+= l_tpALifeItemAmmo->a_elapsed;
			m_dwTotalMoney		-= l_tpALifeItemAmmo->m_dwCost;
		}
	}
	return						(u16(l_dwResult));
}

void CSE_ALifeHumanAbstract::attach_available_ammo(CSE_ALifeItemWeapon *tpALifeItemWeapon, ITEM_P_VECTOR &tpItemVector, OBJECT_VECTOR *tpObjectVector)
{
	if (!tpALifeItemWeapon || !tpALifeItemWeapon->m_caAmmoSections)
		return;
	u32							l_dwCount = 0, l_dwSafeMoney = m_dwTotalMoney;
	ITEM_P_IT					I = tpItemVector.begin();
	ITEM_P_IT					E = tpItemVector.end();
	for ( ; I != E; ++I) {
		CSE_ALifeItemAmmo		*l_tpALifeItemAmmo = smart_cast<CSE_ALifeItemAmmo*>(*I);
		if (l_tpALifeItemAmmo && strstr(tpALifeItemWeapon->m_caAmmoSections,*l_tpALifeItemAmmo->s_name) && (l_tpALifeItemAmmo->m_dwCost <= m_dwTotalMoney) && bfCanGetItem(l_tpALifeItemAmmo) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),l_tpALifeItemAmmo->ID) == tpObjectVector->end()))) {
			if (!tpObjectVector)
				alife().graph().attach(*this,l_tpALifeItemAmmo,l_tpALifeItemAmmo->m_tGraphID);
			else {
				children.push_back(l_tpALifeItemAmmo->ID);
				m_dwTotalMoney -= l_tpALifeItemAmmo->m_dwCost;
			}
			++l_dwCount;
			if (l_dwCount >= MAX_AMMO_ATTACH_COUNT)
				break;
		}
	}
	m_dwTotalMoney				= l_dwSafeMoney;
}

void CSE_ALifeHumanAbstract::vfProcessItems()
{
	alife().m_temp_item_vector.clear();
	D_OBJECT_P_MAP::const_iterator	I = ai().alife().graph().objects()[m_tGraphID].objects().objects().begin();
	D_OBJECT_P_MAP::const_iterator	E = ai().alife().graph().objects()[m_tGraphID].objects().objects().end();
	for ( ; I != E; ++I) {
		CSE_ALifeInventoryItem	*l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>((*I).second);
		if (l_tpALifeInventoryItem && l_tpALifeInventoryItem->bfUseful() && !(*I).second->m_bOnline)
			if ((randF(1.0f) < m_detect_probability)) {
				alife().m_temp_item_vector.push_back(l_tpALifeInventoryItem);
#ifdef DEBUG
				if (psAI_Flags.test(aiALife)) {
					Msg		("[LSS] %s detected item %s on the graph point %d (probability %f, speed %f)",name_replace(),l_tpALifeInventoryItem->base()->name_replace(),m_tGraphID,m_detect_probability,m_fCurSpeed);
				}
#endif
			}
			else {
#ifdef DEBUG
				if (psAI_Flags.test(aiALife)) {
					Msg		("[LSS] %s didn't detect item %s on the graph point %d (probability %f, speed %f)",name_replace(),l_tpALifeInventoryItem->base()->name_replace(),m_tGraphID,m_detect_probability,m_fCurSpeed);
				}
#endif
			}
	}
	if (!alife().m_temp_item_vector.empty())
		vfAttachItems();
}

void CSE_ALifeHumanAbstract::vfDetachAll(bool bFictitious)
{
	while (!children.empty()) {
		CSE_ALifeInventoryItem		*l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(ai().alife().objects().object(*children.begin()));
		R_ASSERT2					(l_tpALifeInventoryItem,"Invalid inventory object");
		if (!bFictitious)
			alife().graph().detach	(*this,l_tpALifeInventoryItem,m_tGraphID);
		else {
			OBJECT_IT				I = children.begin();
			detach					(l_tpALifeInventoryItem,&I);
		}
	}
	R_ASSERT2						((m_fCumulativeItemMass < EPS_L) && !m_iCumulativeItemVolume,"Invalid cumulative item mass or volume value");
}

CSE_ALifeDynamicObject *CSE_ALifeHumanAbstract::tpfGetBestDetector()
{
	m_tpBestDetector				= 0;
	CSE_ALifeGroupAbstract			*l_tpALifeGroupAbstract = smart_cast<CSE_ALifeGroupAbstract*>(this);
	if (l_tpALifeGroupAbstract) {
		u32							l_dwBestValue = 0;
		if (!l_tpALifeGroupAbstract->m_wCount)
			return					(0);
		OBJECT_IT					I = l_tpALifeGroupAbstract->m_tpMembers.begin();
		OBJECT_IT					E = l_tpALifeGroupAbstract->m_tpMembers.end();
		for ( ; I != E; ++I) {
			CSE_ALifeHumanAbstract	*l_tpALifeHumanAbstract = smart_cast<CSE_ALifeHumanAbstract*>(ai().alife().objects().object(l_tpALifeGroupAbstract->m_tpMembers[0]));
			R_ASSERT				(l_tpALifeHumanAbstract);
			ai().ef_storage().alife().member_item() = l_tpALifeHumanAbstract->tpfGetBestDetector();
			u32						l_dwCurrentValue = iFloor(ai().ef_storage().m_pfDetectorType->ffGetValue()+.5f);
			if (l_dwCurrentValue > l_dwBestValue) {
				l_dwBestValue		= l_dwCurrentValue;
				m_tpBestDetector	= const_cast<CSE_ALifeDynamicObject*>(smart_cast<const CSE_ALifeDynamicObject*>(ai().ef_storage().alife().member_item()));
			}
		}
		return						(m_tpBestDetector);
	}
	
	OBJECT_IT						I = children.begin();
	OBJECT_IT						E = children.end();
	for ( ; I != E; ++I) {
		CSE_ALifeDynamicObject		*l_tpALifeDynamicObject = ai().alife().objects().object(*I);
		CSE_ALifeInventoryItem		*l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(l_tpALifeDynamicObject);
		R_ASSERT2					(l_tpALifeInventoryItem,"Non-item object in the inventory found");
		switch (l_tpALifeDynamicObject->m_tClassID) {
			case CLSID_DETECTOR_SIMPLE		: {
				m_tpBestDetector	= l_tpALifeDynamicObject;
				break;
			}
			case CLSID_DETECTOR_VISUAL		: {
				m_tpBestDetector	= l_tpALifeDynamicObject;
				return				(m_tpBestDetector);
			}
		}
	}
	return							(m_tpBestDetector);
}

bool CSE_ALifeHumanAbstract::bfCanGetItem(CSE_ALifeInventoryItem *tpALifeInventoryItem)
{
	if (tpALifeInventoryItem && ((m_fCumulativeItemMass + tpALifeInventoryItem->m_fMass > m_fMaxItemMass) || (m_iCumulativeItemVolume + tpALifeInventoryItem->m_iVolume > MAX_ITEM_VOLUME)))
		return		(false);
	return			(true);
}

bool CSE_ALifeHumanAbstract::bfChooseFast()
{
	// fast check if I can pick up all the items
	u32								l_dwCurrentItemCount = children.size();
	float							l_fCumulativeItemMass = m_fCumulativeItemMass;
	int								l_iCumulativeItemVolume = m_iCumulativeItemVolume;
	bool							l_bOk = true;
	ITEM_P_IT						I = alife().m_temp_item_vector.begin();
	ITEM_P_IT						E = alife().m_temp_item_vector.end(), J = E - 1;
	for ( ; I != E; ++I)
		if ((I != J) || bfCanGetItem(*I)) {
			m_fCumulativeItemMass	+= (*I)->m_fMass;
			m_iCumulativeItemVolume	+= (*I)->m_iVolume;
			children.push_back		((*I)->base()->ID);
		}
		else {
			l_bOk = false;
			break;
		}

	m_fCumulativeItemMass			= l_fCumulativeItemMass;
	m_iCumulativeItemVolume			= l_iCumulativeItemVolume;
	children.resize					(l_dwCurrentItemCount);

	if (l_bOk) {
		I							= alife().m_temp_item_vector.begin();
		for ( ; I != E; ++I)
			alife().graph().attach	(*this,*I,smart_cast<CSE_ALifeDynamicObject*>(*I)->m_tGraphID);
		return						(true);
	}
	
	return							(false);
}

int CSE_ALifeHumanAbstract::ifChooseEquipment(OBJECT_VECTOR *tpObjectVector)
{
	// stalkers cannot change their equipment due to the game design :-((
	return						(0);
//	// choosing equipment
//	CSE_ALifeInventoryItem			*l_tpALifeItemBest	= 0;
//	float							l_fItemBestValue	= -1.f;
//	ai().ef_storage().alife_evaluation(true);
//	ai().ef_storage().alife().member()	= this;
//
//	ITEM_P_IT					I = alife().m_temp_item_vector.begin(), X;
//	ITEM_P_IT					E = alife().m_temp_item_vector.end();
//	for ( ; I != E; ++I) {
//		// checking if it is an equipment item
//		ai().ef_storage().alife().member_item() = smart_cast<CSE_ALifeObject*>(*I);
//		if (ai().ef_storage().m_pfEquipmentType->ffGetValue() > ai().ef_storage().m_pfEquipmentType->ffGetMaxResultValue())
//			continue;
//		if (m_dwTotalMoney < (*I)->m_dwCost)
//			continue;
//		// evaluating item
//		float					l_fCurrentValue = ai().ef_storage().m_pfEquipmentType->ffGetValue();
//		// choosing the best item
//		if ((l_fCurrentValue > l_fItemBestValue) && bfCanGetItem(*I) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),(*I)->base()->ID) == tpObjectVector->end()))) {
//			l_fItemBestValue	= l_fCurrentValue;
//			l_tpALifeItemBest	= *I;
//			X					= I;
//		}
//	}
//	if (l_tpALifeItemBest) {
//		if (!tpObjectVector) {
//			alife().graph().attach	(*this,l_tpALifeItemBest,smart_cast<CSE_ALifeDynamicObject*>(l_tpALifeItemBest)->m_tGraphID);
//			alife().m_temp_item_vector.erase(X);
//		}
//		else
//			children.push_back	(l_tpALifeItemBest->base()->ID);
//		return					(1);
//	}
//	return						(0);
}

int  CSE_ALifeHumanAbstract::ifChooseWeapon(EWeaponPriorityType tWeaponPriorityType, OBJECT_VECTOR *tpObjectVector)
{
	CSE_ALifeInventoryItem	*l_tpALifeItemBest	= 0;
	float					l_fItemBestValue	= -1.f;
	ai().ef_storage().alife_evaluation	(true);
	ai().ef_storage().alife().member()	= this;

	u32						l_dwSafeMoney = m_dwTotalMoney;
	ITEM_P_IT				I = alife().m_temp_item_vector.begin();
	ITEM_P_IT				E = alife().m_temp_item_vector.end();
	for ( ; I != E; ++I) {
		// checking if it is a hand weapon
		ai().ef_storage().alife().member_item() = smart_cast<CSE_ALifeObject*>(*I);
		if (m_dwTotalMoney < (*I)->m_dwCost)
			continue;
		int						j = ai().ef_storage().m_pfPersonalWeaponType->dwfGetWeaponType();
		float					l_fCurrentValue = -1.f;
		switch (tWeaponPriorityType) {
			case eWeaponPriorityTypeKnife : {
				if (1 != j)
					continue;
				l_fCurrentValue = ai().ef_storage().m_pfItemValue->ffGetValue();
				break;
			}
			case eWeaponPriorityTypeSecondary : {
				if (5 != j)
					continue;
				l_fCurrentValue = ai().ef_storage().m_pfSmallWeaponValue->ffGetValue();
				break;
			}
			case eWeaponPriorityTypePrimary : {
				if ((6 != j) && (8 != j) && (9 != j))
					continue;
				l_fCurrentValue = ai().ef_storage().m_pfMainWeaponValue->ffGetValue();
				break;
			}
			case eWeaponPriorityTypeGrenade : {
				if (7 != j)
					continue;
				l_fCurrentValue = ai().ef_storage().m_pfItemValue->ffGetValue();
				break;
			}
			default : NODEFAULT;
		}
		// choosing the best item
		if ((l_fCurrentValue > l_fItemBestValue) && bfCanGetItem(*I) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),(*I)->base()->ID) == tpObjectVector->end()))) {
			l_fItemBestValue = l_fCurrentValue;
			l_tpALifeItemBest = *I;
		}
	}
	if (l_tpALifeItemBest) {
		u32						l_dwCount = children.size();
		
		if (!tpObjectVector)
			alife().graph().attach	(*this,l_tpALifeItemBest,smart_cast<CSE_ALifeDynamicObject*>(l_tpALifeItemBest)->m_tGraphID);
		else
			children.push_back	(l_tpALifeItemBest->base()->ID);
		
		m_dwTotalMoney			-= l_tpALifeItemBest->m_dwCost;
		attach_available_ammo	(smart_cast<CSE_ALifeItemWeapon*>(l_tpALifeItemBest),alife().m_temp_item_vector,tpObjectVector);
		m_dwTotalMoney			= l_dwSafeMoney;
		
		if (!tpObjectVector) {
			ITEM_P_IT				I = remove_if(alife().m_temp_item_vector.begin(),alife().m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
			alife().m_temp_item_vector.erase(I,alife().m_temp_item_vector.end());
		}
		return					(children.size() - l_dwCount);
	}
	return						(0);
}

int  CSE_ALifeHumanAbstract::ifChooseFood(OBJECT_VECTOR *tpObjectVector)
{
#pragma todo("Dima to Dima : Add food and medikit items need count computations")
	// choosing food
	ai().ef_storage().alife_evaluation(true);
	ai().ef_storage().alife().member()	= this;
	u32							l_dwCount = 0, l_dwSafeMoney = m_dwTotalMoney;
	ITEM_P_IT					I = alife().m_temp_item_vector.begin();
	ITEM_P_IT					E = alife().m_temp_item_vector.end();
	for ( ; I != E; ++I) {
		if ((*I)->m_iFoodValue <= 0)
			continue;
		if (m_dwTotalMoney < (*I)->m_dwCost)
			continue;
		if (bfCanGetItem(*I) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),(*I)->base()->ID) == tpObjectVector->end()))) {
			m_dwTotalMoney		-= (*I)->m_dwCost;
			if (!tpObjectVector)
				alife().graph().attach	(*this,*I,smart_cast<CSE_ALifeDynamicObject*>(*I)->m_tGraphID);
			else {
				children.push_back((*I)->base()->ID);
			}
			++l_dwCount;
			if (l_dwCount >= MAX_ITEM_FOOD_COUNT)
				break;
		}
	}
	m_dwTotalMoney				= l_dwSafeMoney;
	if (l_dwCount) {
		if (!tpObjectVector) {
			ITEM_P_IT			I = remove_if(alife().m_temp_item_vector.begin(),alife().m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
			alife().m_temp_item_vector.erase(I,alife().m_temp_item_vector.end());
		}
	}
	return						(l_dwCount);
}

int  CSE_ALifeHumanAbstract::ifChooseMedikit(OBJECT_VECTOR *tpObjectVector)
{
	// choosing medikits
	u32						l_dwCount = 0, l_dwSafeMoney = m_dwTotalMoney;
	ITEM_P_IT				I = alife().m_temp_item_vector.begin();
	ITEM_P_IT				E = alife().m_temp_item_vector.end();
	for ( ; I != E; ++I) {
		if ((*I)->m_iHealthValue <= 0)
			continue;
		if (m_dwTotalMoney < (*I)->m_dwCost)
			continue;
		if (bfCanGetItem(*I) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),(*I)->base()->ID) == tpObjectVector->end()))) {
			m_dwTotalMoney	-= (*I)->m_dwCost;
			if (!tpObjectVector)
				alife().graph().attach	(*this,*I,smart_cast<CSE_ALifeDynamicObject*>(*I)->m_tGraphID);
			else
				children.push_back((*I)->base()->ID);
			++l_dwCount;
			if (l_dwCount >= MAX_ITEM_MEDIKIT_COUNT)
				break;
		}
	}
	m_dwTotalMoney = l_dwSafeMoney;
	if (l_dwCount) {
		if (!tpObjectVector) {
			ITEM_P_IT		I = remove_if(alife().m_temp_item_vector.begin(),alife().m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
			alife().m_temp_item_vector.erase(I,alife().m_temp_item_vector.end());
		}
	}
	return					(l_dwCount);
}

int  CSE_ALifeHumanAbstract::ifChooseDetector(OBJECT_VECTOR *tpObjectVector)
{
	// choosing detector
	CSE_ALifeInventoryItem		*l_tpALifeItemBest	= 0;
	float						l_fItemBestValue	= -1.f;
	ai().ef_storage().alife_evaluation(true);
	ai().ef_storage().alife().member()	= this;
	ITEM_P_IT					I = alife().m_temp_item_vector.begin(), X;
	ITEM_P_IT					E = alife().m_temp_item_vector.end();
	for ( ; I != E; ++I) {
		// checking if it is an item
		CSE_ALifeItemDetector	*l_tpALifeItem = smart_cast<CSE_ALifeItemDetector*>(*I);
		if (!l_tpALifeItem)
			continue;
		if (m_dwTotalMoney < l_tpALifeItem->m_dwCost)
			continue;
		// evaluating item
		ai().ef_storage().alife().member_item() = l_tpALifeItem;
		float					l_fCurrentValue = ai().ef_storage().m_pfEquipmentType->ffGetValue();
		// choosing the best item
		if ((l_fCurrentValue > l_fItemBestValue) && bfCanGetItem(l_tpALifeItem) && (!tpObjectVector || (std::find(tpObjectVector->begin(),tpObjectVector->end(),l_tpALifeItem->ID) == tpObjectVector->end()))) {
			l_fItemBestValue = l_fCurrentValue;
			l_tpALifeItemBest = l_tpALifeItem;
			X = I;
		}
	}
	if (l_tpALifeItemBest) {
		if (!tpObjectVector) {
			alife().graph().attach	(*this,l_tpALifeItemBest,smart_cast<CSE_ALifeDynamicObject*>(l_tpALifeItemBest)->m_tGraphID);
			alife().m_temp_item_vector.erase(X);
		}
		else
			children.push_back(l_tpALifeItemBest->base()->ID);
		return					(1);
	}
	return						(0);
}

int  CSE_ALifeHumanAbstract::ifChooseValuables()
{
	// choosing the rest objects
	ITEM_P_IT				I = alife().m_temp_item_vector.begin();
	ITEM_P_IT				E = alife().m_temp_item_vector.end();
	for ( ; I != E; ++I)
		if (bfCanGetItem(*I))
			alife().graph().attach	(*this,*I,smart_cast<CSE_ALifeDynamicObject*>(*I)->m_tGraphID);

	u32						l_dwCount = children.size();
	I						= remove_if(alife().m_temp_item_vector.begin(),alife().m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
	alife().m_temp_item_vector.erase(I,alife().m_temp_item_vector.end());

	return					(children.size() - l_dwCount);
}

void CSE_ALifeHumanAbstract::vfAttachItems(ETakeType tTakeType)
{
	R_ASSERT2					(fHealth >= EPS_L,"Cannot graph().attach items to dead human");
	
	CSE_ALifeGroupAbstract		*l_tpALifeGroupAbstract = smart_cast<CSE_ALifeGroupAbstract*>(this);
	if (l_tpALifeGroupAbstract) {
		vfChooseGroup			(l_tpALifeGroupAbstract);
		return;
	}
	else
		if (bfChooseFast())
			return;
	
	if (eTakeTypeAll == tTakeType) {
		alife().append_item_vector	(children,alife().m_temp_item_vector);
		vfDetachAll				();
	}
	
	sort						(alife().m_temp_item_vector.begin(),alife().m_temp_item_vector.end(),CSortItemPredicate());

	if ((eTakeTypeAll == tTakeType) || (eTakeTypeMin == tTakeType)) {
		ifChooseFood			();
		ifChooseWeapon			(eWeaponPriorityTypeKnife);
		ifChooseWeapon			(eWeaponPriorityTypeSecondary);
		ifChooseWeapon			(eWeaponPriorityTypePrimary);
		ifChooseWeapon			(eWeaponPriorityTypeGrenade);
		ifChooseMedikit			();
		ifChooseDetector		();
		ifChooseEquipment		();
	}

	if ((eTakeTypeAll == tTakeType) || (eTakeTypeRest == tTakeType))
		ifChooseValuables		();
}
/**/
