// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_BITARRAY_H
#define NV_CORE_BITARRAY_H

#include <nvcore/nvcore.h>
#include <nvcore/Containers.h>

namespace nv
{

/// Count the bits of @a x.
inline uint bitsSet(uint8 x) {
	uint count = 0;
	for(; x != 0; x >>= 1) {
		count += (x & 1);
	}
	return count;
}


/// Count the bits of @a x.
inline uint bitsSet(uint32 x, int bits) {
	uint count = 0;
	for(; x != 0 && bits != 0; x >>= 1, bits--) {
		count += (x & 1);
	}
	return count;
}


/// Simple bit array.
class BitArray
{
public:

	/// Default ctor.
	BitArray() {}

	/// Ctor with initial m_size.
	BitArray(uint sz)
	{
		resize(sz);
	}

	/// Get array m_size.
	uint size() const { return m_size; }

	/// Clear array m_size.
	void clear() { resize(0); }

	/// Set array m_size.
	void resize(uint sz)
	{ 
		m_size = sz;
		m_bitArray.resize( (m_size + 7) >> 3 );
	}
	
	/// Get bit.
	bool bitAt(uint b) const
	{
		nvDebugCheck( b < m_size );
		return (m_bitArray[b >> 3] & (1 << (b & 7))) != 0;
	}

	/// Set a bit.
	void setBitAt(uint b)
	{
		nvDebugCheck( b < m_size );
		m_bitArray[b >> 3] |=  (1 << (b & 7));
	}

	/// Clear a bit.
	void clearBitAt( uint b )
	{
		nvDebugCheck( b < m_size );
		m_bitArray[b >> 3] &= ~(1 << (b & 7));
	}

	/// Clear all the bits.
	void clearAll()
	{
		memset(m_bitArray.unsecureBuffer(), 0, m_bitArray.size());
	}

	/// Set all the bits.
	void setAll()
	{
		memset(m_bitArray.unsecureBuffer(), 0xFF, m_bitArray.size());
	}

	/// Toggle all the bits.
	void toggleAll()
	{
		const uint byte_num = m_bitArray.size();
		for(uint b = 0; b < byte_num; b++) {
			m_bitArray[b] ^= 0xFF;
		}
	}
	
	/// Get a byte of the bit array.
	const uint8 & byteAt(uint index) const
	{
		return m_bitArray[index];
	}

	/// Set the given byte of the byte array.
	void setByteAt(uint index, uint8 b)
	{
		m_bitArray[index] = b;
	}
	
	/// Count the number of bits set.
	uint countSetBits() const
	{
		const uint num = m_bitArray.size();
		if( num == 0 ) {
			return 0;
		}
		
		uint count = 0;				
		for(uint i = 0; i < num - 1; i++) {
			count += bitsSet(m_bitArray[i]);
		}
		count += bitsSet(m_bitArray[num-1], m_size & 0x7);
		
		//piDebugCheck(count + countClearBits() == m_size);
		return count;
	}

	/// Count the number of bits clear.
	uint countClearBits() const {
		
		const uint num = m_bitArray.size();
		if( num == 0 ) {
			return 0;
		}
		
		uint count = 0;
		for(uint i = 0; i < num - 1; i++) {
			count += bitsSet(~m_bitArray[i]);
		}
		count += bitsSet(~m_bitArray[num-1], m_size & 0x7);
		
		//piDebugCheck(count + countSetBits() == m_size);
		return count;
	}

	friend void swap(BitArray & a, BitArray & b)
	{
		swap(a.m_size, b.m_size);
		swap(a.m_bitArray, b.m_bitArray);
	}


private:

	/// Number of bits stored.
	uint m_size;

	/// Array of bits.
	Array<uint8> m_bitArray;

};

} // nv namespace

#endif // _PI_CORE_BITARRAY_H_
