/* Copyright (C) Tom Forsyth, 2001. 
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Tom Forsyth, 2001"
 */
// Some standard useful classes, templates, etc.


#ifndef ArbitraryListH
#define ArbitraryListH


// An arbitrary-sized list template class.
// Designed to hold _unsorted_ data, but only RemoveItem()
// actually disturbs the order, so you can use it for general arrays
// if you don't use that function.
template <class T>
class ArbitraryList
{
	T		*pT;				// The list.
	u32		iSize;				// The current size of the list.
	u32		iReservedSize;		// The current reserved size of the list.
public:

	// Constructor, with optional initial size setting.
	ArbitraryList ( u32 iInitialSize = 0 )
	{
		pT = NULL;
		iSize = 0;
		iReservedSize = 0;
		if ( iInitialSize > 0 )
			resize( iInitialSize );
	}
	// Destructor.
	~ArbitraryList ( void )
	{
		if ( pT == NULL ){
			VERIFY ( iReservedSize == 0 );
			VERIFY ( iSize == 0 );
		}else{
			VERIFY ( iReservedSize > 0 );
			VERIFY ( iSize > 0 );
			delete[] pT;
			pT = NULL;
		}
	}
	// Returns the pointer to the given item.
	IC T*		item		(u32 iItem)
	{
		VERIFY ( iItem < iSize );
		return ( &pT[iItem] );
	}
	// Returns the pointer to the first item.
	IC T*		ptr			( )				{ return (pT);}
	// Returns the size of the list
	IC u32		size		( )	const		{ return iSize;	}
	// Grows or shrinks the list to this number of items.
	// Preserves existing items.
	// Items that fall off the end of a shrink may vanish.
	// Returns the pointer to the first item.
	IC T*		resize		(u32 iNum)
	{
		VERIFY ( iNum >= 0 );
		iSize = iNum;
		if ( iNum <= iReservedSize ){
			if ( iNum == 0 ){
				// Shrunk to 0 - bin the memory.
				delete[] pT;
				pT = NULL;
				iReservedSize = 0;
			}else{
				VERIFY ( pT != NULL );
			}
			return ( pT );
		}else{
			// Need to grow. Grow by 50% more than
			// needed to avoid constant regrows.
			u32 iNewSize = ( iNum * 3 ) >> 1;
			if ( pT == NULL ){
				VERIFY ( iReservedSize == 0 );
				pT = new T [iNewSize];
			}else{
				VERIFY ( iReservedSize != 0 );

				T *pOldT = pT;
				pT = new T[iNewSize];
				for ( u32 i = 0; i < iReservedSize; i++ ){
					pT[i] = pOldT[i];
				}
				delete[] pOldT;
			}
			VERIFY ( pT != NULL );
			iReservedSize = iNewSize;
			return ( pT );
		}
	}
	// Adds one item to the list and returns a pointer to that new item.
	IC T*		append		( )
	{
		resize ( iSize + 1 );
		return ( &pT[iSize-1] );
	}
	// Adds one item to the list and returns a pointer to that new item.
	IC void		push_back	(T& val)
	{
		resize ( iSize + 1 );
		pT[iSize-1]	= val;
	}
	// Removes the given item number by copying the last item
	// to that position and shrinking the list.
	IC void		erase_fast	(u32 iItemNumber)
	{
		VERIFY ( iItemNumber < iSize );
		pT[iItemNumber] = pT[iSize-1];
		resize ( iSize - 1 );
	}
	// Copy the specified data into the list.
	IC void		insert		(u32 iFirstItem, T *p, u32 iNumItems)
	{
		for ( u32 i = 0; i < iNumItems; i++ )
			*(Item ( i + iFirstItem ) ) = p[i];
	}
	// A copy from another arbitrary list of the same type.
	IC void		insert ( u32 iFirstItem, ArbitraryList<T> &other, u32 iFirstOtherItem, u32 iNumItems )
	{
		for ( u32 i = 0; i < iNumItems; i++ )
			*(item ( i + iFirstItem ) ) = *(other.item ( i + iFirstOtherItem ) );
	}
	IC T&		operator[]	(u32 id)		{ VERIFY(id<iSize); return pT[id];	}
	IC const T&	operator[]	(u32 id) const	{ VERIFY(id<iSize); return pT[id];	}

	// Copy constructor.
	ArbitraryList ( const ArbitraryList<T> &other )
	{
		u32 iNumItems = other.size();

		pT = NULL;
		iSize = 0;
		iReservedSize = 0;
		if ( iNumItems > 0 )
			resize ( iNumItems );
		for ( u32 i = 0; i < iNumItems; i++ )
			*(item(i) ) = other[i];
	}
};

#endif //#ifndef ArbitraryListH
