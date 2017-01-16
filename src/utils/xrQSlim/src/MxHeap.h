#ifndef MXHEAP_INCLUDED // -*- C++ -*-
#define MXHEAP_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  Heap

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxHeap.h,v 1.11 2000/11/20 20:36:38 garland Exp $

 ************************************************************************/

#include "MxDynBlock.h"

class MxHeapable
{
private:
    float import;
    int token;

public:
    MxHeapable()
    {
        not_in_heap();
        heap_key(0.0f);
    }

    inline bool is_in_heap() { return token != -47; }
    inline void not_in_heap() { token = -47; }
    inline int get_heap_pos() { return token; }
    inline void set_heap_pos(int t) { token = t; }
    inline void heap_key(float k) { import = k; }
    inline float heap_key() const { return import; }
};

class MxHeap : private MxDynBlock<MxHeapable*>
{
private:
    void place(MxHeapable* x, unsigned int i);
    void swap(unsigned int i, unsigned int j);

    unsigned int parent(unsigned int i) { return (i - 1) / 2; }
    unsigned int left(unsigned int i) { return 2 * i + 1; }
    unsigned int right(unsigned int i) { return 2 * i + 2; }
    void upheap(unsigned int i);
    void downheap(unsigned int i);

public:
    MxHeap() : MxDynBlock<MxHeapable*>(8) {}
    MxHeap(unsigned int n) : MxDynBlock<MxHeapable*>(n) {}
    void insert(MxHeapable* t) { insert(t, t->heap_key()); }
    void insert(MxHeapable*, float);
    void update(MxHeapable* t) { update(t, t->heap_key()); }
    void update(MxHeapable*, float);

    unsigned int size() const { return length(); }
    MxHeapable* item(unsigned int i) { return (*this)[i]; }
    const MxHeapable* item(unsigned int i) const { return (*this)[i]; }
    MxHeapable* extract();
    MxHeapable* top() { return (length() < 1 ? (MxHeapable*)0 : item(0)); }
    MxHeapable* remove(MxHeapable*);
};

// MXHEAP_INCLUDED
#endif
