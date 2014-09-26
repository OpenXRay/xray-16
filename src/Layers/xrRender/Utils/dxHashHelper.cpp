#include "stdafx.h"
#include "dxHashHelper.h"

bool	dxHashHelper::m_bTableReady = false;
u32		dxHashHelper::m_CrcTable[256];	// Lookup table array 

dxHashHelper::dxHashHelper() : m_uiCrcValue(0xffffffff)
{
	if (!m_bTableReady)
	{
		Crc32Init();
		m_bTableReady = true;
	}
}

u32 dxHashHelper::GetHash() const
{
	return m_uiCrcValue ^ 0xffffffff;
}

// Reflects CRC bits in the lookup table
inline u32 dxHashHelper::Reflect (u32 ref, char ch) 
{
	// Used only by Init_CRC32_Table(). 

	u32 value(0); 

	// Swap bit 0 for bit 7 
	// bit 1 for bit 6, etc. 
	for(int i = 1; i < (ch + 1); i++) 
	{ 
		if(ref & 1) 
			value |= 1 << (ch - i); 
		ref >>= 1; 
	} 
	return value; 
} 

void dxHashHelper::Crc32Init()
{
	// Call this function only once to initialize the CRC table. 

	// This is the official polynomial used by CRC-32 
	// in PKZip, WinZip and Ethernet. 
	u32 ulPolynomial = 0x04c11db7; 

	// 256 values representing ASCII character codes. 
	for(int i = 0; i <= 0xFF; i++) 
	{ 
		m_CrcTable[i]=Reflect(i, 8) << 24; 
		for (int j = 0; j < 8; j++) 
			m_CrcTable[i] = (m_CrcTable[i] << 1) ^ (m_CrcTable[i] & (1 << 31) ? ulPolynomial : 0); 
		m_CrcTable[i] = Reflect(m_CrcTable[i], 32); 
	} 
} 

//u32				crc32		(const void* P, u32 len) 
//{
	//if (!crc32_ready)	
	//{
	//	crc32_init	();
	//	crc32_ready	= TRUE;
	//}

	// Pass a text string to this function and it will return the CRC. 

	// Once the lookup table has been filled in by the two functions above, 
	// this function creates all CRCs using only the lookup table. 

	// Be sure to use unsigned variables, 
	// because negative values introduce high bits 
	// where zero bits are required. 

	// Start out with all bits set high. 
	//u32		ulCRC		= 0xffffffff; 
	//u8*		buffer		= (u8*)P;

	// Perform the algorithm on each character 
	// in the string, using the lookup table values. 
	//while(len--) 
	//	ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *buffer++]; 

	// Exclusive OR the result with the beginning value. 
	//return ulCRC ^ 0xffffffff; 
//} 