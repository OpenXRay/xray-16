////////////////////////////////////////////////////////////////////////////
//	Module 		: id_generator.h
//	Created 	: 28.08.2003
//  Modified 	: 28.08.2003
//	Author		: Dmitriy Iassenev and Oles' Shyshkovtsov
//	Description : ID generation class template
////////////////////////////////////////////////////////////////////////////

#pragma once

template<
	typename TIME_ID, 
	typename TYPE_ID, 
	typename VALUE_ID, 
	typename BLOCK_ID, 
	typename CHUNK_ID,
	VALUE_ID tMinValue, 
	VALUE_ID tMaxValue, 
	CHUNK_ID tBlockSize,
	VALUE_ID tInvalidValueID = tMaxValue,
	TIME_ID	 tStartTime = 0> 
class CID_Generator {
private:
	struct SID_Block {
		CHUNK_ID	m_tCount;
		TIME_ID		m_tTimeID;
		TYPE_ID		m_tpIDs[tBlockSize];

		IC				SID_Block	() : m_tCount(0) {}
		
		IC	bool		operator<	(const SID_Block &b) const
		{
			return	(m_tCount && ((m_tTimeID < b.m_tTimeID) || !b.m_tCount));
		}
	};

private:
	enum {
		m_tBlockCount			= u32(tMaxValue - tMinValue)/tBlockSize + 1,
	};

private:
	u32							m_available_count;
	SID_Block					m_tppBlocks	[m_tBlockCount];

private:
	IC		BLOCK_ID			tfGetBlockByValue(VALUE_ID tValueID)
	{
		BLOCK_ID				l_tBlockID = BLOCK_ID((tValueID - tMinValue)/tBlockSize);
		R_ASSERT2				(l_tBlockID < m_tBlockCount,"Requesting ID is invalid!");
		return					(l_tBlockID);
	}

	IC		VALUE_ID			tfGetFromBlock	(SID_Block &l_tID_Block, VALUE_ID tValueID)
	{
		VERIFY					(l_tID_Block.m_tCount);
		BLOCK_ID				l_tBlockID = BLOCK_ID(&l_tID_Block - m_tppBlocks);

		if (l_tID_Block.m_tCount == 1) {
			--m_available_count;
			VERIFY				(m_available_count >= 0);
		}

		if (tInvalidValueID == tValueID)
			return				(VALUE_ID(l_tID_Block.m_tpIDs[--l_tID_Block.m_tCount]) + l_tBlockID*tBlockSize + tMinValue);

		TYPE_ID					*l_tpBlockID = std::find(l_tID_Block.m_tpIDs, l_tID_Block.m_tpIDs + l_tID_Block.m_tCount, TYPE_ID((tValueID - tMinValue)%tBlockSize));	
		R_ASSERT2				(l_tID_Block.m_tpIDs + l_tID_Block.m_tCount != l_tpBlockID,"Requesting ID has already been used!");
		*l_tpBlockID			= *(l_tID_Block.m_tpIDs + --l_tID_Block.m_tCount);
		return					(tValueID);
	}

public:
	IC							CID_Generator	()
	{
		m_available_count		= 0;
		for (VALUE_ID i=tMinValue; ; ++i) {
			vfFreeID			(i,tStartTime);
			if (i >= tMaxValue)
				break;
		}
		VERIFY					(m_available_count == m_tBlockCount);
		for (u32 j=0; j<m_tBlockCount; ++j)
			std::reverse		(m_tppBlocks[j].m_tpIDs,m_tppBlocks[j].m_tpIDs + m_tppBlocks[j].m_tCount);
	}

	IC		VALUE_ID			tfGetID			(VALUE_ID tValueID = tInvalidValueID)
	{
		if (tInvalidValueID != tValueID)
			return				(tfGetFromBlock(m_tppBlocks[tfGetBlockByValue(tValueID)],tValueID));

		R_ASSERT2				(m_available_count,"Not enough IDs");
		SID_Block*				I = std::min_element(m_tppBlocks,m_tppBlocks + m_tBlockCount);
		VERIFY					(I != m_tppBlocks + m_tBlockCount);
		return					(tfGetFromBlock(*I,tValueID));
	}

	IC		void				vfFreeID		(VALUE_ID tValueID, TIME_ID tTimeID)
	{
		BLOCK_ID				l_tBlockID = tfGetBlockByValue(tValueID);
		SID_Block				&l_tID_Block = m_tppBlocks[l_tBlockID];

		VERIFY					(l_tID_Block.m_tCount < tBlockSize);

		if (!l_tID_Block.m_tCount) {
			++m_available_count;
			VERIFY				(m_available_count <= m_tBlockCount);
		}

#ifdef DEBUG
		TYPE_ID					*l_tpBlockID = std::find(l_tID_Block.m_tpIDs, l_tID_Block.m_tpIDs + l_tID_Block.m_tCount, TYPE_ID((tValueID - tMinValue)%tBlockSize));	
		VERIFY					(l_tpBlockID == l_tID_Block.m_tpIDs + l_tID_Block.m_tCount);
#endif
		l_tID_Block.m_tpIDs		[l_tID_Block.m_tCount++] = TYPE_ID((tValueID - tMinValue)%tBlockSize);
		l_tID_Block.m_tTimeID	= tTimeID;
	}
};
