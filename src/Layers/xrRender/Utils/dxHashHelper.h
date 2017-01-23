#ifndef	dxHashHelper_included
#define	dxHashHelper_included
#pragma once

class dxHashHelper
{
public:
	dxHashHelper();
	IC	void	AddData(const void* P, u32 len);
	u32		GetHash() const;
private:
	// Reflects CRC bits in the lookup table 
	static inline u32	Reflect (u32 ref, char ch);
	static void			Crc32Init();
private:
	
	u32		m_uiCrcValue;

	static bool m_bTableReady;
	static u32	m_CrcTable[256];
};

IC void dxHashHelper::AddData(const void* P, u32 len)
{
	u8* buffer = (u8*)P;

	while(len--) 
	{
		m_uiCrcValue = 
			(m_uiCrcValue >> 8) 
			^ m_CrcTable [ (m_uiCrcValue & 0xFF) ^ *buffer++ ];
	}
}

#endif	//	dxHashHelper_included