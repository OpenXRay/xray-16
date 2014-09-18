/**
 * OpenAL cross platform audio library
 * Copyright (C) 1999-2003 by authors.
 * This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA  02111-1307, USA.
 * Or go to http://www.gnu.org/copyleft/lgpl.html
 */




#ifndef _AL_LIST_H_
#define _AL_LIST_H_

#include "openal\al.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>



//*****************************************************************************
//*****************************************************************************
//
// Defines
//
//*****************************************************************************
//*****************************************************************************

//
// Some useful things to track parameters.
//
#ifndef IN
#define IN
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef OUT
#define OUT
#endif

//
// The list entry.  This should not be modified outside the list routines.
//
typedef struct ALlistEntry_Struct
{
    //
    // The previous list entry.
    //
    struct ALlistEntry_Struct* Previous;

    //
    // The next list entry.
    //
    struct ALlistEntry_Struct* Next;

    //
    // The data for the current entry.
    //
    ALvoid* Data;

} ALlistEntry;


//
// This is the context to pass to all the list calls.  It must be initialized
// before any list calls are made.
//
typedef struct //ALlist_Struct
{
    //
    // This is the pointer to the first item in the list.
    //
    ALlistEntry* Head;

    //
    // This is the pointer to the last item in the list.
    //
    ALlistEntry* Tail;

    //
    // This is the list iterator.
    //
    ALlistEntry* Current;

    //
    // This is the list lock to prevent simultaneous addition and removal
    // of entries.
    //
    CRITICAL_SECTION Lock;

    //
    // This maintains a count of the number of entries in the list.
    //
    ALint NumberOfEntries;

    //
    // This is set if the list is locked.  For debug use only.
    //
#if(DBG)
    ALint Locked;
#endif

} ALlist;



//*****************************************************************************
//*****************************************************************************
//
// List Functions
//
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
// alListAddEntry
//*****************************************************************************
// Adds an entry to the tail of the list.  Each entry must be unique.
//
ALvoid alListAddEntry
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
);

//*****************************************************************************
// alListAddEntryToHead
//*****************************************************************************
// Adds an entry to the head of the list.  Each entry must be unique.
//
ALvoid alListAddEntryToHead
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
);

//*****************************************************************************
// alListAcquireLock
//*****************************************************************************
// This is called to acquire the list lock for operations that span multiple
// list calls like iterating over the list.
//
ALvoid alListAcquireLock
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListCreate
//*****************************************************************************
// Creates and initializes a list.
//
ALboolean alListCreate
(
    OUT ALlist** ppList
);

//*****************************************************************************
// alListFree
//*****************************************************************************
// Destroys the list.  Dynamically allocated entries are not freed.
//
ALvoid alListFree
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListGetData
//*****************************************************************************
// Returns the data from the list entry.
//
ALvoid* alListGetData
(
    IN  ALlistEntry* pEntry
);

//*****************************************************************************
// alListGetEntryAt
//*****************************************************************************
// Returns the entry in the list at the specified index of the list.
//
ALlistEntry* alListGetEntryAt
(
    IN  ALlist* pList,
    IN  ALint Index
);

//*****************************************************************************
// alListGetEntryCount
//*****************************************************************************
// Returns the number of items stored in the list.
//
ALint alListGetEntryCount
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListGetHead
//*****************************************************************************
// Returns the first entry in the list.
//
ALlistEntry* alListGetHead
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListGetNext
//*****************************************************************************
// Returns the entry after to the entry pointed to by the iterator.  If
// the iterator is at the last entry (or has finished iterating over the
// list), the returned entry will be 0.
//
ALlistEntry* alListGetNext
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListGetPrevious
//*****************************************************************************
// Returns the entry previous to the entry pointed to by the iterator.  If
// the iterator is at the first entry, the returned entry will be 0.
//
ALlistEntry* alListGetPrevious
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListGetTail
//*****************************************************************************
// Returns the last entry in the list.
//
ALlistEntry* alListGetTail
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListInitializeEntry
//*****************************************************************************
// Initializes a preallocated list entry.
//
ALvoid alListInitializeEntry
(
    IN  ALlistEntry* pListEntry,
    IN  ALvoid* pData
);

//*****************************************************************************
// alListIsEmpty
//*****************************************************************************
// Returns the TRUE if the list is empty.
//
ALboolean alListIsEmpty
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListIteratorGet
//*****************************************************************************
// Returns the entry pointed to by the iterator.
//
ALlistEntry* alListIteratorGet
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListIteratorFindData
//*****************************************************************************
// Searches the list for the matching item and return the pointer to the
// entry.  If the match is not found, the return will be 0.
//
ALlistEntry* alListIteratorFindData
(
    IN  ALlist* pList,
    IN  ALvoid* pData
);

//*****************************************************************************
// alListIteratorNext
//*****************************************************************************
// This is called to advance the list iterator to the next entry in the list
// and return that entry.
//
ALlistEntry* alListIteratorNext
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListIteratorPrevious
//*****************************************************************************
// This is called to advance the list iterator to the previous entry in the
// list and return that entry.
//
ALlistEntry* alListIteratorPrevious
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListIteratorReset
//*****************************************************************************
// Returns the list iterator to the head of the list and returns the head
// entry.
//
ALlistEntry* alListIteratorReset
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListIteratorRemove
//*****************************************************************************
// Removes the current item from the list and returns it.  The iterator will
// equal the next item in the list.
//
ALlistEntry* alListIteratorRemove
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListIteratorSet
//*****************************************************************************
// Sets the current entry pointer to the entry passed in.  If the entry is not
// found, the current entry will be 0.
//
ALlistEntry* alListIteratorSet
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
);

//*****************************************************************************
// alListMatchEntry
//*****************************************************************************
// Matches the entry to an item in the list and returns the data in that
// entry.  If the match is not found, the return will be 0.
//
ALvoid* alListMatchEntry
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
);

//*****************************************************************************
// alListMatchData
//*****************************************************************************
// Searches the list for the matching item and return the pointer to the
// entry.  If the match is not found, the return will be 0.
//
ALlistEntry* alListMatchData
(
    IN  ALlist* pList,
    IN  ALvoid* pData
);

//*****************************************************************************
// alListReleaseLock
//*****************************************************************************
// This is called to release the list lock.
//
ALvoid alListReleaseLock
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListRemoveEntry
//*****************************************************************************
// Removes the item from the list and returns the data from the item.  If
// this is the current item, the current item will equal the next item in the
// list.
//
ALvoid* alListRemoveEntry
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
);

//*****************************************************************************
// alListRemoveHead
//*****************************************************************************
// Removes the list entry at the head of the list.  If this is the current
// item, the current item will equal the next item in the list.
//
ALlistEntry* alListRemoveHead
(
    IN  ALlist* pList
);

//*****************************************************************************
// alListRemoveTail
//*****************************************************************************
// Removes the list entry at the tail of the list.  If this is the current
// item, the current item will be null.
//
ALlistEntry* alListRemoveTail
(
    IN  ALlist* pList
);

#ifdef __cplusplus
}
#endif

#endif
