/************************************************************************

  Heap data structure

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxHeap.cxx,v 1.7 2000/11/20 20:36:38 garland Exp $

 ************************************************************************/
#include "stdafx.h"
#pragma hdrstop

#include "MxHeap.h"

#define ref(i) ((*this)[i])

////////////////////////////////////////////////////////////////////////
//
// Internal functions for manipulating the heap

inline void MxHeap::place(MxHeapable* x, unsigned int i)
{
    ref(i) = x;
    x->set_heap_pos(i);
}

void MxHeap::swap(unsigned int i, unsigned int j)
{
    MxHeapable* tmp = ref(i);

    place(ref(j), i);
    place(tmp, j);
}

void MxHeap::upheap(unsigned int i)
{
    MxHeapable* moving = ref(i);
    unsigned int index = i;
    unsigned int p = parent(i);

    while (index > 0 && moving->heap_key() > ref(p)->heap_key())
    {
        place(ref(p), index);
        index = p;
        p = parent(p);
    }

    if (index != i)
        place(moving, index);
}

void MxHeap::downheap(unsigned int i)
{
    MxHeapable* moving = ref(i);
    unsigned int index = i;
    unsigned int l = left(i);
    unsigned int r = right(i);
    unsigned int largest;

    while (l < (unsigned int)length())
    {
        if (r < (unsigned int)length() && ref(l)->heap_key() < ref(r)->heap_key())
            largest = r;
        else
            largest = l;

        if (moving->heap_key() < ref(largest)->heap_key())
        {
            place(ref(largest), index);
            index = largest;
            l = left(index);
            r = right(index);
        }
        else
            break;
    }

    if (index != i)
        place(moving, index);
}

////////////////////////////////////////////////////////////////////////
//
// Exported interface to the heap
//

void MxHeap::insert(MxHeapable* t, float v)
{
    t->heap_key(v);

    add(t);
    unsigned int i = last_id();
    t->set_heap_pos(i);

    upheap(i);
}

void MxHeap::update(MxHeapable* t, float v)
{
    //    VERIFY( t->is_in_heap() );
    t->heap_key(v);

    unsigned int i = t->get_heap_pos();

    if (i > 0 && v > ref(parent(i))->heap_key())
        upheap(i);
    else
        downheap(i);
}

MxHeapable* MxHeap::extract()
{
    if (length() < 1)
        return 0; // NULL;

    swap(0, length() - 1);
    MxHeapable* dead = drop();

    downheap(0);
    dead->not_in_heap();
    return dead;
}

MxHeapable* MxHeap::remove(MxHeapable* t)
{
    if (!t->is_in_heap())
        return 0; // NULL;

    int i = t->get_heap_pos();
    swap(i, length() - 1);
    drop();
    t->not_in_heap();

    if (ref(i)->heap_key() < t->heap_key())
        downheap(i);
    else
        upheap(i);

    return t;
}
