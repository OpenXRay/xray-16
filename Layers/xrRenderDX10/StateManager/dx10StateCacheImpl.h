#ifndef	dx10StateCacheImpl_included
#define	dx10StateCacheImpl_included
#pragma once

#include "../dx10StateUtils.h"

using dx10StateUtils::operator==;

template <class IDeviceState, class StateDecs>
IDeviceState* 
dx10StateCache<IDeviceState, StateDecs>
::GetState( SimulatorStates& state_code )
{
	StateDecs		desc;
	dx10StateUtils::ResetDescription(desc);
	state_code.UpdateDesc(desc);

	return GetState(desc);

	/*
	u32 crc = crc32( &desc, sizeof(desc));

	pResult = FindState( desc, crc);

	if (!pResult)
	{
		StateRecord rec;
		rec.m_crc = crc;
		CreateState(desc, &rec.m_pState);
		pResult = rec.m_pState;
		m_StateArray.push_back(rec);
	}

	return pResult;
	*/
}

template <class IDeviceState, class StateDecs>
IDeviceState* 
dx10StateCache<IDeviceState, StateDecs>
::GetState( StateDecs& desc )
{
	IDeviceState*	pResult;

	dx10StateUtils::ValidateState(desc);

	u32 crc = dx10StateUtils::GetHash(desc);

	pResult = FindState( desc, crc);

	if (!pResult)
	{
		StateRecord rec;
		rec.m_crc = crc;
		CreateState(desc, &rec.m_pState);
		pResult = rec.m_pState;
		m_StateArray.push_back(rec);
	}

	return pResult;
}

template <class IDeviceState, class StateDecs>
IDeviceState* 
dx10StateCache<IDeviceState, StateDecs>
::FindState( const StateDecs& desc, u32 StateCRC )
{
    u32 res = 0xffffffff;
	for (u32 i=0; i<m_StateArray.size(); ++i)
	{
		if (m_StateArray[i].m_crc==StateCRC)
		{
			StateDecs	descCandidate;
			m_StateArray[i].m_pState->GetDesc(&descCandidate);
			//if ( !memcmp(&descCandidate, &desc, sizeof(desc)) )
			if (descCandidate==desc)
				//break;
			//	TEST
			{
                res = i;
				break;
			}
			//else
			//{
			//	VERIFY(0);
			//}
		}
	}

	if (res!=0xffffffff)
		return m_StateArray[res].m_pState;
	else
		return NULL;
    /*
	if (i!=m_StateArray.size())
		return m_StateArray[i].m_pState;
	else
		return NULL;
        */
}

#endif	//	dx10StateCacheImpl_included