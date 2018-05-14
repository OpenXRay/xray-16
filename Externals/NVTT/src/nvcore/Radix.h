///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *	Contains source code from the article "Radix Sort Revisited".
 *	\file		Radix.h
 *	\author		Pierre Terdiman
 *	\date		April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef NV_CORE_RADIXSORT_H
#define NV_CORE_RADIXSORT_H

#include <nvcore/nvcore.h>


#define RADIX_LOCAL_RAM


class NVCORE_API RadixSort {
	NV_FORBID_COPY(RadixSort);
public:
	// Constructor/Destructor
	RadixSort();
	~RadixSort();

	// Sorting methods
	RadixSort & sort(const uint32* input, uint32 nb, bool signedvalues=true);
	RadixSort & sort(const float* input, uint32 nb);

	//! Access to results. mIndices is a list of indices in sorted order, i.e. in the order you may further process your data
	inline uint32 * indices() const { return mIndices; }

	//! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
	inline uint32 * recyclable() const { return mIndices2; }

	// Stats
	uint32 usedRam() const;

	//! Returns the total number of calls to the radix sorter.
	inline uint32 totalCalls()	const { return mTotalCalls;	}

	//! Returns the number of premature exits due to temporal coherence.
	inline uint32 hits() const { return mNbHits; }


	private:
#ifndef RADIX_LOCAL_RAM
	uint32*			mHistogram;					//!< Counters for each byte
	uint32*			mOffset;					//!< Offsets (nearly a cumulative distribution function)
#endif
	uint32			mCurrentSize;				//!< Current size of the indices list
	uint32			mPreviousSize;				//!< Size involved in previous call
	uint32*			mIndices;					//!< Two lists, swapped each pass
	uint32*			mIndices2;

	// Stats
	uint32			mTotalCalls;
	uint32			mNbHits;

	// Internal methods
	bool			resize(uint32 nb);
	void			resetIndices();

};


#endif // NV_CORE_RADIXSORT_H
