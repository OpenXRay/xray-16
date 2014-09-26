////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_wrappers.h
//	Created 	: 20.10.2004
//  Modified 	: 20.10.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry wrappers
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_registry_container.h"
#include "alife_registry_wrapper.h"

//typedef CALifeRegistryWrapper<CInfoPortionRegistry> KNOWN_INFO_REGISTRY;

//реестр контактов общения с другими персонажами
//typedef CALifeRegistryWrapper<CKnownContactsRegistry> KNOWN_CONTACTS_REGISTRY;

//реестр статей энциклопедии, о которых знает актер
//typedef CALifeRegistryWrapper<CEncyclopediaRegistry> ENCYCLOPEDIA_REGISTRY;

//реестр заданий, полученных актером
//typedef CALifeRegistryWrapper<CGameTaskRegistry> GAME_TASK_REGISTRY;

//реестр новостей, полученных актером
//typedef CALifeRegistryWrapper<CGameNewsRegistry> GAME_NEWS_REGISTRY;


template <typename T>
class CALifeRegistryWrapperObject {
	T				*m_registry;

public:
	IC				CALifeRegistryWrapperObject		()
	{
		m_registry	= xr_new<T>();
	}

	virtual			~CALifeRegistryWrapperObject	()
	{
		xr_delete	(m_registry);
	}

	IC		T		&registry						() const
	{
		VERIFY		(m_registry);
		return		(*m_registry);
	}
};

//class CKnownContactsRegistryWrapper :	public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CKnownContactsRegistry> > {};
//class CEncyclopediaRegistryWrapper :	public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CEncyclopediaRegistry> > {};
class CGameNewsRegistryWrapper :		public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CGameNewsRegistry> > {};
class CInfoPortionWrapper :				public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CInfoPortionRegistry> > {};
class CRelationRegistryWrapper :		public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CRelationRegistry> > {};
class CMapLocationWrapper :				public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CMapLocationRegistry> > {};
class CGameTaskWrapper :				public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CGameTaskRegistry> > {};

//. class CFogOfWarWrapper :				public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CFogOfWarRegistry> > {};
class CActorStatisticsWrapper :			public CALifeRegistryWrapperObject<CALifeRegistryWrapper<CActorStatisticRegistry> > {};
