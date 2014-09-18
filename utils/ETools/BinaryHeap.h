#ifndef BinaryHeapH
#define BinaryHeapH
// A binary heap. Actually stores pointers to type THING, not the THINGs themselves.
template <class THING, class SORT> class BinaryHeap
{

private:
	struct Blob
	{
		THING				*pThing;
		SORT				Sort;
	};

	Blob					*pBlobArray;
	int						iCurrentSize;
	int						iAllocatedSize;

	int						iCurrentPos;

#ifdef DEBUG
	// A flag that says using FindNext, RemoveNext and RemoveCurrent are OK to call.
	bool					bFindNextValid;
#endif


public:

	BinaryHeap ( void )
	{
		pBlobArray = NULL;
		iCurrentSize = 0;
		iAllocatedSize = 0;
#ifdef DEBUG
		bFindNextValid = FALSE;
#endif
		iCurrentPos = 0;
	}

	~BinaryHeap ( void )
	{
		if ( pBlobArray != NULL )
		{
			delete[] pBlobArray;
			pBlobArray = NULL;
		}
		iCurrentSize = 0;
		iAllocatedSize = 0;
	}


	void Check ( void )
	{
#if CHECK_HEAP
#ifdef DEBUG
		for ( int iCurPos = iCurrentSize-1; iCurPos >= 2; iCurPos-- )
		{
			VERIFY ( pBlobArray[iCurPos].Sort <= pBlobArray[iCurPos>>1].Sort );
			VERIFY ( pBlobArray[iCurPos].pThing != NULL );
		}
#endif
#endif
	}

	void Add ( THING *pThisThing, SORT ThisSort )
	{
		Check();

#ifdef DEBUG
		bFindNextValid = FALSE;
#endif
		iCurrentPos = 0;

		if ( iCurrentSize <= 1 )
		{
			iCurrentSize = 1;
		}

		// Make sure it's big enough.
		if ( iAllocatedSize <= iCurrentSize )
		{
			// Add 50% to the allocated size.
			iAllocatedSize = ( iAllocatedSize >> 1 ) + iAllocatedSize;
			// And then a bit more.
			iAllocatedSize += 32;
			// And round up to 1k array size.
			iAllocatedSize = ( iAllocatedSize + 1023 ) & ~1023;

			Blob *pOldBlobArray = pBlobArray;
			pBlobArray = new Blob[iAllocatedSize];
			VERIFY ( pBlobArray != NULL );
			if ( pOldBlobArray != NULL )
			{
				memcpy ( pBlobArray, pOldBlobArray, ( iCurrentSize + 1 ) * sizeof ( Blob ) );
				delete[] pOldBlobArray;
			}
			Check();
		}

		// And add the item.
		iCurrentPos = iCurrentSize;
		while ( ( iCurrentPos > 1 ) && ( pBlobArray[iCurrentPos >> 1].Sort < ThisSort ) )
		{
			pBlobArray[iCurrentPos] = pBlobArray[iCurrentPos >> 1];
			iCurrentPos >>= 1;
		}
		pBlobArray[iCurrentPos].Sort = ThisSort;
		pBlobArray[iCurrentPos].pThing = pThisThing;

		iCurrentSize++;

		Check();
	}


	THING *FindFirst ( void )
	{
		if ( iCurrentSize > 1 )
		{
			VERIFY ( pBlobArray != NULL );
#ifdef DEBUG
			bFindNextValid = TRUE;
#endif
			iCurrentPos = 1;
			return pBlobArray[1].pThing;
		}
		else
		{
#ifdef DEBUG
			bFindNextValid = FALSE;
#endif
			iCurrentPos = 0;
			return NULL;
		}
	}

	// Must have called FindFirst first.
	// THIS DOES NOT TRAVERSE IN SORTED ORDER!
	THING *FindNextUnsorted ( void )
	{
#ifdef DEBUG
		VERIFY ( bFindNextValid );
#endif
		if ( iCurrentPos >= iCurrentSize - 1 )
		{
			// Reached the end.
			return NULL;
		}
		else
		{
			iCurrentPos++;

			return pBlobArray[iCurrentPos].pThing;
		}
	}

	// Must have called FindFirst/FindNext first.
	SORT GetCurrentSort ( void )
	{
#ifdef DEBUG
		VERIFY ( bFindNextValid );
#endif
		VERIFY ( iCurrentPos < iCurrentSize );
		VERIFY ( pBlobArray != NULL );
		return pBlobArray[iCurrentPos].Sort;
	}

	// Must have called FindFirst/FindNext first.
	THING *RemoveCurrent ( void )
	{
#ifdef DEBUG
		VERIFY ( bFindNextValid );
		bFindNextValid = FALSE;
#endif
		if ( iCurrentPos < ( iCurrentSize - 1 ) )
		{
			VERIFY ( pBlobArray != NULL );
			THING *pThing = pBlobArray[iCurrentPos].pThing;

			SORT MovedSort = pBlobArray[iCurrentSize-1].Sort;


			// First bubble this item up the list until
			// the parent is greater or equal to the last item in the heap.
			while ( ( iCurrentPos > 1 ) &&
				( pBlobArray[iCurrentPos>>1].Sort < MovedSort ) )
			{
				pBlobArray[iCurrentPos] = pBlobArray[iCurrentPos>>1];
				iCurrentPos = iCurrentPos >> 1;
			}



			// Then delete it, and replace it by the last in the heap,
			// then bubble that item down the heap again.
			iCurrentSize--;

			// And bubble the last item back down the tree.
			while ( (iCurrentPos<<1) < iCurrentSize )
			{
				if ( ( MovedSort >= pBlobArray[(iCurrentPos<<1)+0].Sort ) &&
					( ( ((iCurrentPos<<1)+1) >= iCurrentSize ) ||
					( MovedSort >= pBlobArray[(iCurrentPos<<1)+1].Sort ) ) )
				{
					// Yep - fits here.
					break;
				}
				else
				{
					// Find the bigger of the two, and move it up.
					if ( ( ((iCurrentPos<<1)+1) < iCurrentSize ) &&
						( pBlobArray[(iCurrentPos<<1)+0].Sort < pBlobArray[(iCurrentPos<<1)+1].Sort ) )
					{
						pBlobArray[iCurrentPos] = pBlobArray[(iCurrentPos<<1)+1];
						iCurrentPos = (iCurrentPos<<1)+1;
					}
					else
					{
						pBlobArray[iCurrentPos] = pBlobArray[(iCurrentPos<<1)+0];
						iCurrentPos = (iCurrentPos<<1)+0;
					}
				}
			}

			// Fits here.
			pBlobArray[iCurrentPos] = pBlobArray[iCurrentSize];
			pBlobArray[iCurrentSize].pThing = NULL;

			Check();

			return pThing;
		}
		else if ( iCurrentPos == iCurrentSize - 1 )
		{
			// This is already the last item - that was easy!
			iCurrentSize--;
			THING *pThing = pBlobArray[iCurrentPos].pThing;
			return pThing;
		}
		else
		{
			return NULL;
		}
	}

	// Must have called FindFirst first.
	THING *RemoveNext ( void )
	{
#ifdef DEBUG
		VERIFY ( bFindNextValid );
#endif
		iCurrentPos++;
		return RemoveCurrent();
	}

	THING *RemoveFirst ( void )
	{
#ifdef DEBUG
		// Keep the assert happy.
		bFindNextValid = TRUE;
#endif
		iCurrentPos = 1;
		return RemoveCurrent();
	}

	// Number of items in the heap.
	int SizeOfHeap ( void )
	{
		return ( iCurrentSize - 1 );
	}

};

#endif