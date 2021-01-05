#pragma once

/************************************************************************

MxDynBlocks are blocks that automatically grow to fit the data added
to them.

Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

$Id: MxDynBlock.h,v 1.14.2.1 2004/07/01 18:38:41 garland Exp $

************************************************************************/

#include "MxBlock.h"

template <class T>
class MxDynBlock : public MxBlock<T>
{
private:
    int fill;
    using MxBlock<T>::resize;
    using MxBlock<T>::begin;

public:
    MxDynBlock(int n = 2) : MxBlock<T>(n) { fill = 0; }
    int length() const { return fill; }
    int total_space() const { return MxBlock<T>::length(); }
    int last_id() const { return fill - 1; }
    T& last() { return (*this)[last_id()]; }
    const T& last() const { return (*this)[last_id()]; }
    void room_for(int len)
    {
        if (length() < len)
            resize(len);
        fill = len;
    }

    T& add()
    {
        if (length() == total_space())
            resize(total_space() * 2);
        fill++;
        return last();
    }

    void add(const T& t) { add() = t; }
    void reset() { fill = 0; }
    T& drop() { return (*this)[--fill]; }
    void drop(int d) { fill -= d; }
    void remove(int i) { (*this)[i] = (*this)[--fill]; }
#if defined(XR_PLATFORM_WINDOWS) // Не буду удалять потому как не понимаю как оно собирается на винде
    void remove_inorder(int i) { Memory.mem_move(&(*this)[i], &(*this)[i + 1], (--fill - i) * sizeof(T)); }
#endif
    // Restricted STL-like interface for interoperability with
    // STL-based code.  Overrides select MxBlock<> definitions and
    // introduces some additional std::vector-like methods.
    //
    int size() const { return length(); }
    typename MxBlock<T>::iterator end() { return begin() + size(); }
    typename MxBlock<T>::const_iterator end() const { return begin() + size(); }
    void push_back(const T& t) { add(t); }
};

template <class T, int T_SIZE>
class MxSizedDynBlock : public MxDynBlock<T>
{
public:
    MxSizedDynBlock(unsigned int n = T_SIZE) : MxDynBlock<T>(n) {}
};

template <class T>
inline bool varray_find(const MxDynBlock<T>& A, const T& t, unsigned int* index = NULL)
{
    for (unsigned int i = 0; i < (unsigned int)A.length(); i++)
        if (A[i] == t)
        {
            if (index)
                *index = i;
            return true;
        }
    return false;
}
