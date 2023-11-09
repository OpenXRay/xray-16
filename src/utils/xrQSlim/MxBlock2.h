#ifndef MXBLOCK2_INCLUDED // -*- C++ -*-
#define MXBLOCK2_INCLUDED
#if !defined(__GNUC__)
#pragma once
#endif

/************************************************************************

  MxBlock2 provides typed access to 2D data blocks.

  Copyright (C) 1998 Michael Garland.  See "COPYING.txt" for details.

  $Id: MxBlock2.h,v 1.10.2.1 2004/07/01 18:38:41 garland Exp $

 ************************************************************************/

#include "MxBlock.h"

template <class T>
class MxBlock2 : public MxBlock<T>
{
private:
    u32 W;
    u32 H;

protected:
    MxBlock2() {}
public:
    MxBlock2(u32 w, u32 h) : MxBlock<T>(w * h)
    {
        W = w;
        H = h;
    }

    T& operator()(u32 i, u32 j) { return (*this)[j * W + i]; }
    const T& operator()(u32 i, u32 j) const { return (*this)[j * W + i]; }
    u32 width() const { return W; }
    u32 height() const { return H; }
    void resize(u32 w, u32 h)
    {
        W = w;
        H = h;
        MxBlock<T>::resize_block(w * h);
    }
};

// MXBLOCK2_INCLUDED
#endif
