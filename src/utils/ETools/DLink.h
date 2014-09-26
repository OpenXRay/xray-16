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


#ifndef DLinkH
#define DLinkH


// Defines a link
#define DlinkDefine(classname,linkname)\
	classname *linkname##pNextItem;\
	classname *linkname##pPrevItem


// Defines the class methods for the link.
#define DlinkMethods(classname,linkname)																		\
	void linkname##Init ( void ) { linkname##pNextItem = NULL; linkname##pPrevItem = NULL; }					\
	\
	/* Find the next in the list. */																			\
	classname *linkname##Next ( void )																			\
	{																											\
	VERIFY ( ( linkname##pPrevItem == NULL ) || ( linkname##pPrevItem->linkname##pNextItem == this ) );		\
	VERIFY ( ( linkname##pNextItem == NULL ) || ( linkname##pNextItem->linkname##pPrevItem == this ) );		\
	return linkname##pNextItem;																				\
	}																											\
	\
	/* Find the previous in the list. */																		\
	classname *linkname##Prev ( void )																			\
	{																											\
	VERIFY ( ( linkname##pPrevItem == NULL ) || ( linkname##pPrevItem->linkname##pNextItem == this ) );		\
	VERIFY ( ( linkname##pNextItem == NULL ) || ( linkname##pNextItem->linkname##pPrevItem == this ) );		\
	return linkname##pPrevItem;																				\
	}																											\
	\
	/* Remove this item from the list. */																		\
	/* Returns the next item. */																				\
	classname *linkname##Del ( void )																			\
	{																											\
	classname *pNext = linkname##pNextItem;																	\
	classname *pPrev = linkname##pPrevItem;																	\
	VERIFY ( ( pPrev == NULL ) || ( pPrev->linkname##pNextItem == this ) );									\
	VERIFY ( ( pNext == NULL ) || ( pNext->linkname##pPrevItem == this ) );									\
	if ( pPrev != NULL )																					\
		{																										\
		pPrev->linkname##pNextItem = pNext;																	\
		}																										\
		if ( pNext != NULL )																					\
		{																										\
		pNext->linkname##pPrevItem = pPrev;																	\
		}																										\
		linkname##pNextItem = NULL;																				\
		linkname##pPrevItem = NULL;																				\
		return pNext;																							\
	}																											\
	\
	/* Remove this item from the list. */																		\
	/* Assumes that this is the first item in the list for speed (asserted). */									\
	/* Returns the next item. */																				\
	classname *linkname##DelScan ( void )																		\
	{																											\
	VERIFY ( linkname##pPrevItem == NULL );																	\
	classname *pNext = linkname##pNextItem;																	\
	VERIFY ( ( pNext == NULL ) || ( pNext->linkname##pPrevItem == this ) );									\
	if ( pNext != NULL )																					\
		{																										\
		pNext->linkname##pPrevItem = NULL;																	\
		}																										\
		linkname##pNextItem = NULL;																				\
		linkname##pPrevItem = NULL;																				\
		return pNext;																							\
	}																											\
	\
	/* Deletes the item, given a pointer to the first item in the list. */										\
	/* Modifies the point as necessary. */																		\
	/* Returns the next item. */																				\
	/* Use like   pThing->DelFromFirst ( &pFirst ); */															\
	classname *linkname##DelFromFirst ( classname **ppFirst )													\
	{																											\
	VERIFY ( ppFirst != NULL );																				\
	VERIFY ( *ppFirst != NULL );																			\
	if ( *ppFirst == this )																					\
		{																										\
		VERIFY ( linkname##pPrevItem == NULL );																\
		/* First in the list. */																			\
		*ppFirst = linkname##Del();																			\
		return *ppFirst;																					\
		}																										\
		else																									\
		{																										\
		/* DLink lists already know their previous item. */													\
		return ( linkname##Del() );																			\
		}																										\
	}																											\
	\
	/* Add the item after the given item. */																	\
	/* Return the new next item. */																				\
	classname *linkname##AddAfter ( classname *pPrev )															\
	{																											\
	VERIFY ( linkname##pNextItem == NULL );																	\
	VERIFY ( linkname##pPrevItem == NULL );																	\
	VERIFY ( pPrev != NULL );																				\
	linkname##pNextItem = pPrev->linkname##pNextItem;														\
	linkname##pPrevItem = pPrev;																			\
	if ( linkname##pNextItem != NULL )																		\
		{																										\
		linkname##pNextItem->linkname##pPrevItem = this;													\
		}																										\
		pPrev->linkname##pNextItem = this;																		\
		return ( linkname##pNextItem );																			\
	}																											\
	\
	/* Add the item after the given item. */																	\
	/* Assumes this is the last item in the list for speed (asserted) */										\
	void linkname##AddAfterEnd ( classname *pPrev )																\
	{																											\
	VERIFY ( linkname##pNextItem == NULL );																	\
	VERIFY ( linkname##pPrevItem == NULL );																	\
	VERIFY ( pPrev != NULL );																				\
	VERIFY ( pPrev->linkname##pNextItem == NULL );															\
	linkname##pNextItem = NULL;																				\
	linkname##pPrevItem = pPrev;																			\
	pPrev->linkname##pNextItem = this;																		\
	}																											\
	\
	/* Add before given item. */																				\
	/* Returns the new previous item. */																		\
	classname *linkname##AddBefore ( classname *pNext )															\
	{																											\
	VERIFY ( linkname##pNextItem == NULL );																	\
	VERIFY ( linkname##pPrevItem == NULL );																	\
	VERIFY ( pNext != NULL );																				\
	linkname##pNextItem = pNext;																			\
	linkname##pPrevItem = pNext->linkname##pPrevItem;														\
	if ( linkname##pPrevItem != NULL )																		\
		{																										\
		linkname##pPrevItem->linkname##pNextItem = this;													\
		}																										\
		pNext->linkname##pPrevItem = this;																		\
		return ( linkname##pPrevItem );																			\
	}																											\
	\
	/* Add the item before the given item. */																	\
	/* Assumes this is the first item in the list for speed (asserted) */										\
	void linkname##AddBeforeStart ( classname *pNext )															\
	{																											\
	VERIFY ( linkname##pNextItem == NULL );																	\
	VERIFY ( linkname##pPrevItem == NULL );																	\
	VERIFY ( pNext != NULL );																				\
	VERIFY ( pNext->linkname##pPrevItem == NULL );															\
	linkname##pPrevItem = NULL;																				\
	linkname##pNextItem = pNext;																			\
	pNext->linkname##pPrevItem = this;																		\
	}																											\
	\
	/* Find the last item in the list. */																		\
	classname *linkname##FindLast ( void )																		\
	{																											\
	classname *pCurr = this;																				\
	VERIFY ( pCurr != NULL );																				\
	while ( pCurr->linkname##pNextItem != NULL )															\
		{																										\
		VERIFY ( ( pCurr->linkname##pPrevItem == NULL ) || ( pCurr->linkname##pPrevItem->linkname##pNextItem == pCurr ) );	\
		VERIFY ( ( pCurr->linkname##pNextItem == NULL ) || ( pCurr->linkname##pNextItem->linkname##pPrevItem == pCurr ) );	\
		pCurr = pCurr->linkname##pNextItem;																	\
		}																										\
		return pCurr;																							\
	}																											\
	\
	/* Find the first item in the list. */																		\
	classname *linkname##FindFirst ( void )																		\
	{																											\
	classname *pCurr = this;																				\
	VERIFY ( pCurr != NULL );																				\
	while ( pCurr->linkname##pPrevItem != NULL )															\
		{																										\
		VERIFY ( ( pCurr->linkname##pPrevItem == NULL ) || ( pCurr->linkname##pPrevItem->linkname##pNextItem == pCurr ) );	\
		VERIFY ( ( pCurr->linkname##pNextItem == NULL ) || ( pCurr->linkname##pNextItem->linkname##pPrevItem == pCurr ) );	\
		pCurr = pCurr->linkname##pPrevItem;																	\
		}																										\
		return pCurr;																							\
	}																											\
	\
	/* Consistency check - checks the current list is sound. */													\
	void linkname##ConsistencyCheck ( void )																	\
	{																											\
	classname *pCurr = this;																						\
	/* Scan backwards. */																					\
	VERIFY ( pCurr != NULL );																				\
	while ( pCurr->linkname##pPrevItem != NULL )															\
		{																										\
		VERIFY ( ( pCurr->linkname##pPrevItem == NULL ) || ( pCurr->linkname##pPrevItem->linkname##pNextItem == pCurr ) );				\
		VERIFY ( ( pCurr->linkname##pNextItem == NULL ) || ( pCurr->linkname##pNextItem->linkname##pPrevItem == pCurr ) );				\
		pCurr = pCurr->linkname##pPrevItem;																	\
		}																										\
		VERIFY ( ( pCurr->linkname##pPrevItem == NULL ) || ( pCurr->linkname##pPrevItem->linkname##pNextItem == pCurr ) );					\
		VERIFY ( ( pCurr->linkname##pNextItem == NULL ) || ( pCurr->linkname##pNextItem->linkname##pPrevItem == pCurr ) );					\
		\
		/* Scan forwards. */																					\
		pCurr = this;																							\
		VERIFY ( pCurr != NULL );																				\
		while ( pCurr->linkname##pNextItem != NULL )															\
		{																										\
		VERIFY ( ( pCurr->linkname##pPrevItem == NULL ) || ( pCurr->linkname##pPrevItem->linkname##pNextItem == pCurr ) );				\
		VERIFY ( ( pCurr->linkname##pNextItem == NULL ) || ( pCurr->linkname##pNextItem->linkname##pPrevItem == pCurr ) );				\
		pCurr = pCurr->linkname##pNextItem;																	\
		}																										\
		VERIFY ( ( pCurr->linkname##pPrevItem == NULL ) || ( pCurr->linkname##pPrevItem->linkname##pNextItem == pCurr ) );					\
		VERIFY ( ( pCurr->linkname##pNextItem == NULL ) || ( pCurr->linkname##pNextItem->linkname##pPrevItem == pCurr ) );					\
	}																											\
	\
	/* Adds the item to the end of this list */																	\
	void linkname##AddToEnd ( classname *pItem )																\
	{																											\
	VERIFY ( pItem != NULL );																				\
	linkname##AddAfterEnd ( pItem->linkname##FindLast() );													\
	}																											\
	\
	/* Adds the item the the start of this list */																\
	void linkname##AddToStart ( classname *pItem )																\
	{																											\
	VERIFY ( pItem != NULL );																				\
	linkname##AddBeforeStart ( pItem->linkname##FindFirst() );												\
	}																											\
	\
	/* Check that all the pointer are NULL, ready to be deleted */												\
	bool linkname##CheckNull ( void )																			\
	{																											\
	return ( ( linkname##pNextItem == NULL ) && ( linkname##pPrevItem == NULL ) );							\
	}																											\
	\
	/* Calls delete() on all the objects in this list and deletes it */											\
	void linkname##DelWholeList ( void )																		\
	{																											\
	classname *pCur = linkname##FindFirst();																\
	while ( pCur != NULL )																					\
		{																										\
		classname *pNext = pCur->linkname##DelScan();														\
		delete ( pCur );																					\
		pCur = pNext;																						\
		}																										\
	}																											\

#endif //#ifndef DLinkH
