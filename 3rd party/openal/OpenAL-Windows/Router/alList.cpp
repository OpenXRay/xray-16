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




#include "alList.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>


//****************************************************************************
//****************************************************************************
//
// Defines
//
//****************************************************************************
//****************************************************************************

//
// Some occasionally useful debugging stuff.
//
#if(DBG)
    #ifndef ASSERT
    static ALint alListDebugAssertsEnabled = 1;
    #define ASSERT(exp)                                                     \
        {                                                                   \
            if(!(exp) && alListDebugAssertsEnabled)                         \
            {                                                               \
                char tempStr[256];                                          \
                OutputDebugString("\n");                                    \
                sprintf(tempStr, "Assert failed in file %s, line %d!\n",    \
                        __FILE__, __LINE__);                                \
                OutputDebugString(tempStr);                                 \
                OutputDebugString("\n");                                    \
                if(alListDebugAssertsEnabled)                               \
                {                                                           \
                    DebugBreak();                                           \
                }                                                           \
            }                                                               \
        }

    #endif
#else
    #ifndef ASSERT
    #define ASSERT(exp)
    #endif
#endif



//****************************************************************************
//****************************************************************************
//
// List Functions
//
//****************************************************************************
//****************************************************************************

//*****************************************************************************
// alListAddEntry
//*****************************************************************************
// Adds an entry to the tail of the list.  Each entry must be unique.
//
ALvoid alListAddEntry
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
)
{
#if(DBG)
    ALlistEntry* pCurrent = 0;
#endif

    ASSERT(pList);
    ASSERT(pEntry);
    ASSERT(pList->Locked);
    ASSERT(!pEntry->Next);
    ASSERT(!pEntry->Previous);

    //
    // Verify the entry doesn't already exist.
    //
#if(DBG)
    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent == pEntry)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    if(pCurrent)
    {
        // Duplicate entries are not supported.
        ASSERT(0);
        return;
    }
#endif


    //
    // Add the item to the tail of the list.
    //
    if(pList->Tail)
    {
        pList->Tail->Next = pEntry;
    }

    pEntry->Previous = pList->Tail;
    pList->Tail = pEntry;

    //
    // Check if this is the first entry.
    //
    if(!pList->Head)
    {
        pList->Head = pEntry;
        pList->Current = pEntry;
    }

    pList->NumberOfEntries++;
}


//*****************************************************************************
// alListAddEntryToHead
//*****************************************************************************
// Adds an entry to the head of the list.  Each entry must be unique.
//
ALvoid alListAddEntryToHead
(
    IN  ALlist* pList,
    IN  ALlistEntry* pEntry
)
{
#if(DBG)
    ALlistEntry* pCurrent = 0;
#endif
    ASSERT(pList);
    ASSERT(pEntry);
    ASSERT(pList->Locked);
    ASSERT(!pEntry->Next);
    ASSERT(!pEntry->Previous);

    //
    // Verify the entry doesn't already exist.
    //
#if(DBG)
    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent == pEntry)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    if(pCurrent)
    {
        // Duplicate entries are not supported.
        ASSERT(0);
        return;
    }
#endif


    //
    // Add the item to the head of the list.
    //
    if(pList->Head)
    {
        pList->Head->Previous = pEntry;
    }

    pEntry->Next = pList->Head;
    pList->Head = pEntry;

    //
    // Check if this is the first entry.
    //
    if(!pList->Tail)
    {
        pList->Tail = pEntry;
        pList->Current = pEntry;
    }

    pList->NumberOfEntries++;
}


//*****************************************************************************
// alListAcquireLock
//*****************************************************************************
// This is called to aquire the list lock for operations that span multiple
// list calls like iterating over the list.
//
ALvoid alListAcquireLock
(
    IN  ALlist* pList
)
{
    ASSERT(pList);

    EnterCriticalSection(&pList->Lock);
#if(DBG)
    pList->Locked++;
#endif

    //
    // Check that only one person has the lock.
    //
    ASSERT(pList->Locked == 1);
}


//*****************************************************************************
// alListCreate
//*****************************************************************************
// Creates and initializes a list.
//
ALboolean alListCreate
(
    OUT ALlist** ppList
)
{
    ALlist* pList = 0;

    ASSERT(ppList);

    //
    // Allocate and initialize the list context.
    //
    *ppList = 0;
    pList = (ALlist*)malloc(sizeof(ALlist));
    if(!pList)
    {
        // Failed to allocate the list!
        ASSERT(0);
        return FALSE;
    }

    memset(pList, 0, sizeof(ALlist));
    InitializeCriticalSection(&pList->Lock);
    pList->NumberOfEntries = 0;
    *ppList = pList;
    return TRUE;
}


//*****************************************************************************
// alListFree
//*****************************************************************************
// Destroys the list.
//
ALvoid alListFree
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(!pList->Head);

    //
    // Free the resources allocated during the creation.
    //
    if(pList)
    {
        DeleteCriticalSection(&pList->Lock);
        free(pList);
    }

    return;
}


//*****************************************************************************
// alListGetData
//*****************************************************************************
// Returns the data from the list entry.
//
ALvoid* alListGetData
(
    IN  ALlistEntry* pEntry
)
{
    ASSERT(pEntry);

    return pEntry->Data;
}


//*****************************************************************************
// alListGetEntryAt
//*****************************************************************************
// Returns the entry in the list at the specified index of the list.
//
ALlistEntry* alListGetEntryAt
(
    IN  ALlist* pList,
    IN  ALint Index
)
{
    ALlistEntry* pEntry = 0;
    ALint i;

    ASSERT(pList);
    ASSERT(pList->Locked);
    ASSERT(Index < pList->NumberOfEntries);

    pEntry = pList->Head;
    for(i = 0; i < Index && pEntry; i++)
    {
        pEntry = pEntry->Next;
    }

    return pEntry;
}


//*****************************************************************************
// alListGetEntryCount
//*****************************************************************************
// Returns the number of items stored in the list.
//
ALint alListGetEntryCount
(
    IN  ALlist* pList
)
{
    ASSERT(pList->Locked);
    return pList->NumberOfEntries;
}


//*****************************************************************************
// alListGetHead
//*****************************************************************************
// Returns the first entry in the list.
//
ALlistEntry* alListGetHead
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    return pList->Head;
}


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
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    if(!pList->Current)
    {
        return 0;
    }

    return pList->Current->Next;
}


//*****************************************************************************
// alListGetPrevious
//*****************************************************************************
// Returns the entry previous to the entry pointed to by the iterator.  If
// the iterator is at the first entry, the returned entry will be 0.
//
ALlistEntry* alListGetPrevious
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    if(!pList->Current)
    {
        return 0;
    }

    return pList->Current->Previous;
}


//*****************************************************************************
// alListGetTail
//*****************************************************************************
// Returns the last entry in the list.
//
ALlistEntry* alListGetTail
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    return pList->Tail;
}


//*****************************************************************************
// alListInitializeEntry
//*****************************************************************************
// Initializes a preallocated list entry.
//
ALvoid alListInitializeEntry
(
    ALlistEntry* pEntry,
    ALvoid* pData
)
{
    ASSERT(pEntry);

    pEntry->Data = pData;
    pEntry->Next = 0;
    pEntry->Previous = 0;
}


//*****************************************************************************
// alListIsEmpty
//*****************************************************************************
// Returns the TRUE if the list is empty.
//
ALboolean alListIsEmpty
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    return (pList->Head == 0);
}


//*****************************************************************************
// alListIteratorGet
//*****************************************************************************
// Returns the entry pointed to by the iterator.
//
ALlistEntry* alListIteratorGet
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    return pList->Current;
}


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
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    //
    // Find the item.
    //
    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent->Data == pData)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    pList->Current = pCurrent;
    return pCurrent;
}


//*****************************************************************************
// alListIteratorNext
//*****************************************************************************
// This is called to advance the list iterator to the next entry in the list
// and return that entry.
//
ALlistEntry* alListIteratorNext
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    if(!pList->Current)
    {
        return 0;
    }

    pList->Current = pList->Current->Next;
    return pList->Current;
}


//*****************************************************************************
// alListIteratorPrevious
//*****************************************************************************
// This is called to advance the list iterator to the previous entry in the
// list and return that entry.
//
ALlistEntry* alListIteratorPrevious
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    if(!pList->Current)
    {
        return 0;
    }

    pList->Current = pList->Current->Previous;
    return pList->Current;
}


//*****************************************************************************
// alListIteratorRemove
//*****************************************************************************
// Removes the current item from the list and returns it.  The iterator will
// equal the next item in the list.
//
ALlistEntry* alListIteratorRemove
(
    IN  ALlist* pList
)
{
    ALlistEntry* pEntry = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    //
    // Make sure we aren't at the end of the list.
    //
    if(!pList->Current)
    {
        return 0;
    }

    //
    // Remove the item from the list.
    //
    pEntry = pList->Current;

    //
    // Fix up the next item in the list.
    //
    if(pEntry->Next)
    {
        pEntry->Next->Previous = pEntry->Previous;
    }

    //
    // Fix up the previous item in the list.
    //
    if(pEntry->Previous)
    {
        pEntry->Previous->Next = pEntry->Next;
    }

    //
    // Fix up the current pointer.
    //
    pList->Current = pEntry->Next;

    //
    // Check the head pointer.
    //
    if(pList->Head == pEntry)
    {
        pList->Head = pEntry->Next;
    }

    //
    // Check the tail pointer.
    //
    if(pList->Tail == pEntry)
    {
        pList->Tail = pEntry->Previous;
    }

    //
    // Set the entry pointers.
    //
    pEntry->Next = 0;
    pEntry->Previous = 0;
    pList->NumberOfEntries--;
    ASSERT(0 <= pList->NumberOfEntries);
    return pEntry;
}


//*****************************************************************************
// alListIteratorReset
//*****************************************************************************
// Returns the list iterator to the head of the list.
//
ALlistEntry* alListIteratorReset
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

    pList->Current = pList->Head;
    return pList->Current;
}


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
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent == pEntry)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    pList->Current = pCurrent;
    return pList->Current;
}


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
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    //
    // Find the item.
    //
    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent == pEntry)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    if(!pCurrent)
    {
        return 0;
    }

    return pCurrent->Data;
}


//*****************************************************************************
// alListMatchData
//*****************************************************************************
// Searches the list for the first matching item and returns the pointer to
// the entry.  If the match is not found, the return will be 0.
//
ALlistEntry* alListMatchData
(
    IN  ALlist* pList,
    IN  ALvoid* pData
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    //
    // Find the item.
    //
    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent->Data == pData)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    return pCurrent;
}


//*****************************************************************************
// alListReleaseLock
//*****************************************************************************
// This is called to release the list lock.
//
ALvoid alListReleaseLock
(
    IN  ALlist* pList
)
{
    ASSERT(pList);
    ASSERT(pList->Locked);

#if(DBG)
    pList->Locked--;
    ASSERT(pList->Locked == 0);
#endif

    LeaveCriticalSection(&pList->Lock);
}


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
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);
    ASSERT(pEntry);

    //
    // Release the item from the list.
    //
    pCurrent = pList->Head;
    while(pCurrent)
    {
        if(pCurrent == pEntry)
        {
            break;
        }

        pCurrent = pCurrent->Next;
    }

    if(!pCurrent)
    {
        return 0;
    }

    //
    // Fix up the next item in the list.
    //
    if(pEntry->Next)
    {
        pEntry->Next->Previous = pEntry->Previous;
    }

    //
    // Fix up the previous item in the list.
    //
    if(pEntry->Previous)
    {
        pEntry->Previous->Next = pEntry->Next;
    }

    //
    // Fix up the current pointer.
    //
    if(pCurrent == pList->Current)
    {
        pList->Current = pEntry->Next;
    }

    //
    // Check the head pointer.
    //
    if(pList->Head == pEntry)
    {
        pList->Head = pEntry->Next;
    }

    //
    // Check the tail pointer.
    //
    if(pList->Tail == pEntry)
    {
        pList->Tail = pEntry->Previous;
    }

    //
    // Set the entry pointers.
    //
    pEntry->Next = 0;
    pEntry->Previous = 0;
    pList->NumberOfEntries--;
    ASSERT(0 <= pList->NumberOfEntries);
    return pEntry->Data;
}


//*****************************************************************************
// alListRemoveHead
//*****************************************************************************
// Removes the list entry at the head of the list.  If this is the current
// item, the current item will equal the next item in the list.
//
ALlistEntry* alListRemoveHead
(
    IN  ALlist* pList
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    //
    // Release the item from the list.
    //
    pCurrent = pList->Head;
    if(!pCurrent)
    {
        return 0;
    }

    //
    // Fix up the next item in the list.
    //
    if(pCurrent->Next)
    {
        pCurrent->Next->Previous = 0;
    }

    //
    // Fix up the previous item in the list.
    //
    ASSERT(!pCurrent->Previous)

    //
    // Fix up the current pointer.
    //
    if(pCurrent == pList->Current)
    {
        pList->Current = pCurrent->Next;
    }

    //
    // Check the head pointer.
    //
    pList->Head = pCurrent->Next;

    //
    // Check the tail pointer.
    //
    if(pList->Tail == pCurrent)
    {
        pList->Tail = 0;
    }

    //
    // Set the entry pointers.
    //
    pCurrent->Next = 0;
    pCurrent->Previous = 0;
    pList->NumberOfEntries--;
    ASSERT(0 <= pList->NumberOfEntries);
    return pCurrent;
}


//*****************************************************************************
// alListRemoveTail
//*****************************************************************************
// Removes the list entry at the tail of the list.  If this is the current
// item, the current item will be null.
//
ALlistEntry* alListRemoveTail
(
    IN  ALlist* pList
)
{
    ALlistEntry* pCurrent = 0;

    ASSERT(pList);
    ASSERT(pList->Locked);

    //
    // Release the item from the list.
    //
    pCurrent = pList->Tail;
    if(!pCurrent)
    {
        return 0;
    }

    //
    // Fix up the next item in the list.
    //
    ASSERT(!pCurrent->Next)

    //
    // Fix up the previous item in the list.
    //
    if(pCurrent->Previous)
    {
        pCurrent->Previous->Next = 0;
    }

    //
    // Fix up the current pointer.
    //
    if(pCurrent == pList->Current)
    {
        pList->Current = 0;
    }

    //
    // Check the head pointer.
    //
    if(pList->Head == pCurrent)
    {
        pList->Head = 0;
    }

    //
    // Check the tail pointer.
    //
    pList->Tail = pCurrent->Previous;

    //
    // Set the entry pointers.
    //
    pCurrent->Next = 0;
    pCurrent->Previous = 0;
    pList->NumberOfEntries--;
    ASSERT(0 <= pList->NumberOfEntries);
    return pCurrent;
}

